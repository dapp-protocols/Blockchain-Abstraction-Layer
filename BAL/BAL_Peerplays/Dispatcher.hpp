#pragma once

#include <graphene/protocol/custom.hpp>

#include <BAL/Declarations.hpp>
#include <BAL/IO.hpp>

#include <Utils/TypeList.hpp>

#include <boost/iostreams/stream.hpp>

#include <fc/io/json.hpp>
#include <fc/io/buffered_iostream.hpp>

#include <optional>
#include <variant>
#include <cctype>

const static uint8_t MAX_UNPACK_RECURSION = 20;

namespace BAL {
class Contract;

namespace impl {
class StringViewIstream : public fc::istream {
    std::string_view buffer;

public:
    StringViewIstream(std::string_view buffer) : buffer(buffer) {}

    size_t bytesRemaining() const { return buffer.size(); }

    // istream interface
    size_t readsome(char* buf, size_t len) override {
        FC_ASSERT(!buffer.empty(), "Cannot read from empty buffer!");

        size_t bytesRead = std::min(buffer.size(), len);
        memcpy(buf, buffer.data(), bytesRead);
        buffer.remove_prefix(bytesRead);
        return bytesRead;
    }
    size_t readsome(const std::shared_ptr<char>& buf, size_t len, size_t offset) override {
        // Assuming len is size of entire buffer, not just buffer following offset
        if (offset > len)
            return 0;
        return readsome(buf.get() + offset, len-offset);
    }
    char get() override {
        FC_ASSERT(!buffer.empty(), "Cannot read from empty buffer!");
        char result = buffer.front();
        buffer.remove_prefix(1);
        return result;
    }
};
}

class Dispatcher {
public:
    // A class enumerating the possible outcomes of parsing
    enum class ParseResult {
        // Everything parsed OK
        ParseOK,
        // The parse failed with an error
        ParseFailed,
        // The parse succeeded, but left unparsed data in the operation
        ExtraData,
        // Parsing aborted due to incorrect magic number
        WrongMagic
    };

    using Storage = std::vector<char>;

    virtual ~Dispatcher() = default;

    virtual ParseResult parse(std::string_view contractName, const graphene::protocol::custom_operation& operation,
                              Storage& storage) = 0;
    virtual bool dispatch(Contract& contract, const Storage& storage) = 0;
};

template<typename Contract, typename=void>
class ContractDispatcher : public Dispatcher {
    static_assert(Impl::ContractIsDeclared<Contract>(),
                  "Cannot create dispatcher for contract: contract does not declare its actions");
    using ActionDescriptions = typename ContractDeclarations<Contract>::Actions;

    using StorageVariant =
        Util::TypeList::apply<Util::TypeList::transform<ActionDescriptions, Impl::actionToArguments>, std::variant>;

    ParseResult parseBinary(std::string_view buffer, Storage& storage) const {
        // Parse binary action buffer. Format is:
        // [ <action discriminant: unsigned variable length int> <arguments: binary serialization> ]
        namespace BIO = boost::iostreams;
        namespace TL = Util::TypeList;

        fc::unsigned_int discriminant = std::numeric_limits<fc::unsigned_int>::max();
        BIO::stream<BIO::basic_array_source<char>> argumentStream(buffer.data(), buffer.size());

        // Unpack the discriminant, which tells us which action to invoke
        fc::raw::unpack(argumentStream, discriminant, 1);
        if (discriminant.value < TL::length<ActionDescriptions>()) {
            try {
                // Prepare storage
                storage.resize(sizeof(StorageVariant));
                auto variantStorage = new (storage.data()) StorageVariant();

                // Enter a scope which knows the specific action being invoked
                TL::runtime::Dispatch(TL::index<ActionDescriptions>(), discriminant.value,
                                      [this, &argumentStream, &variantStorage](auto ad) {
                    constexpr size_t Index = TL::first<typename decltype(ad)::type>();
                    using Description = TL::last<typename decltype(ad)::type>;

                    Log("[Parse] Action specified:", Description::ActionName.to_string());

                    // Create a tuple to store the arguments, then loop over the argument types unpacking them
                    // into said tuple
                    typename Description::Arguments tuple;
                    TL::runtime::ForEach(TL::index<typename Description::Parameters>(),
                                         [&tuple, &argumentStream](auto parameter) {
                        constexpr size_t Index = TL::first<typename decltype(parameter)::type>();
                        using ArgumentType = TL::last<typename decltype(parameter)::type>;
                        ArgumentType argument;
                        fc::raw::unpack(argumentStream, argument, MAX_UNPACK_RECURSION);
                        std::get<Index>(tuple) = std::move(argument);
                    });
                    // Store the tuple of arguments for evaluation
                    variantStorage->template emplace<Index>(std::move(tuple));
                });

                // Sanity check: We should be exactly at the end of the buffer. If we aren't, report it
                if (argumentStream.eof() || (std::size_t)argumentStream.tellg() < buffer.size())
                    return ParseResult::ExtraData;
            } catch (fc::exception_ptr e) {
                Log("[Parse] Caught exception while unpacking action arguments from operation:",
                    e->to_detail_string());
                return ParseResult::ParseFailed;
            }
        } else {
            // Invalid discriminant! Log it and return
            Log("[Parse] Operation bore our magic number, but discriminant was invalid:", discriminant);
            return ParseResult::ParseFailed;
        }

        return ParseResult::ParseOK;
    }

    // For parsing textual action buffers, a map of action name to parser function for that action
    using TextParserMap = std::map<Name, std::function<ParseResult(std::string_view, StorageVariant&)>>;
    // A function to create a ParserMap with a parser for each action in the contract
    static TextParserMap makeArgumentStringParsers() {
        namespace TL = Util::TypeList;

        TextParserMap map;

        TL::runtime::ForEach(ActionDescriptions(), [&map] (auto a){
            using ActionDescription = typename decltype(a)::type;
            using ParameterList = typename ActionDescription::Parameters;

            map[ActionDescription::ActionName] = [](std::string_view buffer, StorageVariant& storage) {
                auto argumentStream = std::make_shared<impl::StringViewIstream>(buffer);
                fc::variants arguments;
                constexpr auto parameterCount = TL::length<ParameterList>();
                typename ActionDescription::Arguments tuple;

                // If we expect any arguments...
                if constexpr (parameterCount > 0) {
                    // Read the arguments from the stream
                    fc::buffered_istream bufferedStream(argumentStream);
                    arguments = fc::json::from_stream(bufferedStream).get_array();
                    // Check that an adequate number of arguments were provided
                    if (arguments.size() < parameterCount) {
                        Log("[Parse] Arguments parsed OK, but some were missing!");
                        return ParseResult::ParseFailed;
                    }

                    // For each parameter...
                    TL::runtime::ForEach(TL::index<ParameterList>(), [&tuple, &arguments](auto parameter) {
                        constexpr size_t Index = TL::first<typename decltype(parameter)::type>();
                        using ParameterType = TL::last<typename decltype(parameter)::type>;
                        // Parse the argument to the parameter type and store it
                        std::get<Index>(tuple) = arguments[Index].as<ParameterType>(MAX_UNPACK_RECURSION);
                    });
                }

                constexpr auto ActionIndex = TL::indexOf<ActionDescriptions, ActionDescription>();
                storage.template emplace<ActionIndex>(std::move(tuple));

                if (arguments.size() > parameterCount) {
                    Log("[Parse] Arguments parsed successfully, but more were provided than expected");
                    return ParseResult::ExtraData;
                }
                if (argumentStream->bytesRemaining() > 0) {
                    Log("[Parse] Arguments parsed successfully, but unused data was leftover:", std::string(buffer));
                    return ParseResult::ExtraData;
                }

                return ParseResult::ParseOK;
            };
        });

        return map;
    }

    ParseResult parseString(std::string_view buffer, Storage& storage) const {
        // Parse string action buffer. Format is:
        // [ <whitespace?> <action name: text> <whitespace?> <arguments?: JSON array>

        while (!buffer.empty() && std::isspace(buffer.front()))
            buffer.remove_prefix(1);

        if (buffer.empty()) {
            Log("[Parse] Empty action buffer! Parse failed.");
            return ParseResult::ParseFailed;
        }

        // Read action name. It cannot contain whitespace, and must be followed by it, so it's easy to pick out.
        uint64_t actionNameSize = 0;
        while (buffer.size() > actionNameSize && !isspace(buffer[actionNameSize]))
            actionNameSize++;

        // Read action name and check if it matches any action
        std::string actionNameString(buffer.data(), actionNameSize);
        buffer.remove_prefix(actionNameSize);
        Log("[Parse] Parsed action name:", actionNameString);

        try {
            Name actionName(actionNameString);
            static TextParserMap parsers = makeArgumentStringParsers();

            // Find the parser for this action
            auto parserItr = parsers.find(actionName);
            if (parserItr == parsers.end()) {
                Log("[Parse] Provided action name", actionName.to_string(), "does not match any action in contract!");
                return ParseResult::ParseFailed;
            }
            auto parser = parserItr->second;

            // Prepare argument storage
            storage.resize(sizeof(StorageVariant));
            auto variantStorage = new (storage.data()) StorageVariant();

            return parser(buffer, *variantStorage);
        } catch (fc::exception_ptr e) {
            Log("[Parse] An error occurred while parsing action buffer:", e->to_detail_string());
            return ParseResult::ParseFailed;
        }
    }

public:
    ParseResult parse(std::string_view contractName, const graphene::protocol::custom_operation& op,
                      Storage& storage) override {
        storage.clear();
        // Copy the buffer to a string, and then use string_view from there so we guarantee a null terminator.
        std::string bufferString(op.data.data(), op.data.size());
        std::string_view buffer(bufferString);
        auto magic = "contract-action/" + std::string(contractName) + "-1.0";

        // Does the op's data buffer have the magic number?
        if (op.data.size() < magic.size() || buffer.substr(0, magic.size()) != magic)
            return ParseResult::WrongMagic;

        // After the magic is a character indicating the argument format. "s" means string, "b" means binary
        buffer.remove_prefix(magic.size());
        if (buffer.size() == 0) {
            Log("[Parse] Valid magic, but empty action buffer!");
            return ParseResult::ParseFailed;
        }
        if (buffer.front() == 'b') {
            buffer.remove_prefix(1);
            return parseBinary(buffer, storage);
        } else if (buffer.front() == 's') {
            buffer.remove_prefix(1);
            return parseString(buffer, storage);
        } else {
            Log("[Parse] Valid magic, but missing parse mode!");
            return ParseResult::ParseFailed;
        }
    }
    bool dispatch(BAL::Contract& contract, const Storage& storage) override {
        Verify(storage.size() >= sizeof(StorageVariant), "Dispatch aborted: Storage is too small to read!");
        auto variantStorage = reinterpret_cast<const StorageVariant&>(*storage.data());

        try {
            namespace TL = Util::TypeList;
            TL::runtime::Dispatch(TL::index<ActionDescriptions>(), (uint64_t)variantStorage.index(),
                                  [&contract, &variantStorage](auto action) {
                constexpr size_t Index = TL::first<typename decltype(action)::type>();
                using ActionDescription = TL::last<typename decltype(action)::type>;
                auto CallAction = [&contract, &variantStorage](const auto&... args) {
                    ActionDescription::Call(static_cast<Contract&>(contract), args...);
                };
                std::apply(CallAction, std::get<Index>(variantStorage));
            });
        } catch (fc::exception_ptr e) {
            Log("[Dispatch] An error occurred while executing the action:", *e);
            return false;
        }

        return true;
    }
};

template<typename Contract>
class ContractDispatcher<Contract,
                         std::enable_if_t<
                            Util::TypeList::length<typename Contract::Actions>() == 0>> : public Dispatcher {
public:
    ParseResult parse(std::string_view, const graphene::protocol::custom_operation&, Storage& storage) override {
        storage.clear();
        return ParseResult::WrongMagic;
    }
    bool dispatch(BAL::Contract&, const Storage&) override {
        return false;
    }
};

} // namespace BAL
