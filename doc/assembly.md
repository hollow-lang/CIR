# CIR Assembly (CAS) Overview

CIR Assembly (CAS) is a **function-based assembly language**. It does **not allow global labels**—all code must exist inside functions.

## Function Declaration

```asm
.fn <function_name>
; function body
.end
```


- `.fn <function_name>` — Begins a function named `<function_name>`.
- `.end` — Ends the function.
- **All code and labels must exist inside functions.**

## Local Labels

You **can** declare labels inside a function:

```asm
my_label:
; instructions
```


- To reference a label, prefix it with `@`:

```asm
jmp @my_label
```

> Labels are **local to their function**. They cannot be used outside the function they are defined in.

## Operands and Syntax

CAS uses **typed operand prefixes** for clarity:

| Prefix | Description | Example |
|--------|------------|---------|
| `$`    | Number (integer or float) | `$42`, `$3.14` |
| `"`    | String literal | `"Hello, World!"` |
| (none) | Identifier / syntax sugar | `my_id` → internally treated like `"my_id"` |

> Identifiers are mainly syntactic sugar for instructions like `call`, `callx`, etc. The assembler converts them internally to strings.

## Instructions

All instructions follow this general format:

<instruction_name> <to>, <from>


- `<instruction_name>` — Operation to perform.
- `<to>` — Destination operand.
- `<from>` — Source operand.

> For detailed argument rules and instruction behavior, see:  
> `core/cir.h: execute_instruction`
