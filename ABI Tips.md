# Overview

A BAL contract that will be deployed to a Leap backend must be paired with an [Application Binary Interface (ABI)](https://developers.eos.io/manuals/eosio.cdt/latest/best-practices/abi/understanding-abi-files) during deployment.  The following tips are intended to assist those developers who will manually construct their own ABI.

# Tips for Creating a Leap ABI for a BAL contract

- ABI action names MUST match their declaration in the BAL contract's C++ header DESCRIBE_ACTION names

- The struct name in the ABI that represents a method signature CAN BE named independently of the method name declared in the BAL C++ header (e.g. the contract's C++ method named "getString()" may be name "GetStringMethodSignature" in the ABI)

- An optional type from the C++ contract (e.g. `void methodName(..., optional<string> argumentB ...) {`) MUST BE represented with a "?" suffix applied to the type.  For example:

```
    ...
        {
            "name": "methodSignature",
            "base": "",
            "fields": [
                ...
                {"name": "argumentB", "type": "string?"}
                ...
            ]
        }
    ...
```

- Variant types from the C++ contract MUST BE declared as

```
    ...
    "variants": [
        {"name": "Adjustment", "types": ["uint32","int32"]}
    ],
    ...
```

They may subsequently be used as a type when declaring a type for a method or table.  For example:

```
    ...
        {
            "name": "methodSignature",
            "base": "",
            "fields": [
                ...
                {"name": "argumentC", "type": "Adjustment"}
                ...
            ]
        }
    ...
```


- Maps of object pairs from the C++ contract (e.g. `map<InventoryId, uint32>`) MUST BE declared as

```
    ...
    "structs": [
        ...
        {
            "name": "PickListPair",
            "base": "",
            "fields": [
                {"name":"key", "type":"InventoryId"}
                {"name":"value", "type":"uint32"}
            ]
        }
        ...
    ]
    ...
```

They may subsequently be used as a type when declaring a type for a method or table.  For example:

```
    ...
        {
            "name": "methodSignature",
            "base": "",
            "fields": [
                ...
                {"name": "argumentD", "type": "PickListPair[]"}
                ...
            ]
        }
    ...
```

- Aliases for any type can be declared in the ABI as:

```
    ...
    "types": [
        ...
        {"new_type_name": "PickList", "type": "PickListPair[]"},
        ...
    ],
    ...
```

They may subsequently be used as their own type when declaring a type for a method or table.  For example:


```
    ...
        {
            "name": "methodSignature",
            "base": "",
            "fields": [
                ...
                {"name": "argumentD", "type": "PickList"}
                ...
            ]
        }
    ...
```

