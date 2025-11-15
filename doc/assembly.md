# CIR Assembly (CAS) Language Reference

CIR Assembly (CAS) is a **function-based assembly language** where all code must be contained within functions. Global
labels are not permitted.

---

## External Functions

External functions can be called using the `callx` instruction.
They always exist if registered by a library, but you can also require an extern to be present at runtime.

```asm
.extern example
```

When a required function is not found, CIR will report an error like this:

```text
./build/cas ./something.cas
[INFO] Compiling: ./something.cas
[SUCCESS] Bytecode written to: something.cbc (N bytes)
[INFO] Executing program
[ERROR] Execution failed: Missing required external function: example
```

## Function Structure

Functions are declared using the `.fn` directive and terminated with `.end`:

Functions can have optional attributes:

- `inline` - inlines the function see inline-functions example

```asm
.fn function_name <attributes>
    ; function body
.end
```

**Key Points:**

- `.fn <name>` declares a function
- `.end` marks the end of the function
- All operations must be inside a function
- A `main` function is required as the program entry point

---

## Local Labels

Labels can be defined within functions for control flow:

```asm
.fn my_function
loop_start:
    ; instructions here
    jmp @loop_start
.end
```

**Label Rules:**

- Declared with `label_name:`
- Referenced with `@label_name` prefix
- Scoped to their containing function only
- Cannot be accessed from outside their function
- Optional `.` prefix when declaring: `.loop_start:` (automatically stripped)

---

## Operand Types and Prefixes

CAS uses prefix notation to distinguish operand types:

| Prefix | Type                | Example                            | Description                                         |
|--------|---------------------|------------------------------------|-----------------------------------------------------|
| `$`    | Numeric literal     | `$42`, `$3.14`, `$0xFF`, `$0b1010` | Integer or floating-point value (mandatory)         |
| `"`    | String literal      | `"Hello"`                          | Text string                                         |
| `@`    | Label reference     | `@loop_start`                      | Reference to a local label (mandatory)              |
| `#`    | Explicit identifier | `#my_var`                          | Force treat as identifier/string (optional)         |
| `r`    | Register            | `r0`, `r1`, ...                    | Register reference (some operations check for them) |
| `'`    | Character literal   | `'a'`, `'\n'`                      | Single character (converted to integer)             |

**Special Values:**

- `true`, `TRUE` - Boolean true
- `false`, `FALSE` - Boolean false
- `null`, `NULL` - Null value

### Numeric Literals

All numbers **must** be prefixed with `$`:

```asm
mov $42, r0        ; move 42 into r0
mov $3.14, r1      ; move 3.14 into r1
mov $0xFF, r2      ; hexadecimal (0x prefix)
mov $0b1010, r3    ; binary (0b prefix)
mov $077, r4       ; octal (leading 0)
mov $-123, r5      ; negative number
mov $1.5e-10, r6   ; scientific notation
```

### Compile-Time Expressions

The `comp()` function evaluates expressions at assembly time:

```asm
.fn compute
loop_start:
    mov comp(loop_start + 5), r0   ; in comp for labels you dont need @
    mov comp(10 * 3 + 2), r1       ; Arithmetic: 32
    jmp @end
end:
.end
```

**Features:**

- Supports: `+`, `-`, `*`, `/`, parentheses
- Can reference labels (converted to their instruction offsets)
- Returns floating-point result

---

## Registers

CAS provides 256 registers (`r0` through `r255`):

- `r0` - Default destination for most operations
- `r1-r255` - General-purpose registers

**Note:** `r0` is special - many arithmetic operations automatically store their result in `r0`.

---

## Comments

Comments start with `;` and continue to the end of the line:

```asm
; This is a comment
mov $42, r0  ; Inline comment
```

Lines starting with `#` are also treated as comments.

---

## Example Program

```asm
.fn main
    mov $10, r0
    mov $20, r1
    iadd r0, r1     ; r0 = r0 + r1 = 30

    mov $0, r255    ; this is here because icmp requires 2 registers to compare

loop_start:
    call #helper
    dec r0           ; r0--
    icmp r0, r255
    jne @loop_start  ; Loop while r0 != 0

    ret
.end

.fn helper
    callx #std.print ; prints r0 to stdout
    ret
.end
```

---

## Error Handling

The assembler will report errors with line numbers for:

- Undefined labels
- Duplicate function/label definitions
- Invalid operand syntax
- Missing `.end` directives
- Instructions outside functions
- Invalid register numbers
- Missing `main` function
- Empty programs

---

## Bytecode Format

Assembled programs are compiled to a bytecode format (`.cbc` files):

**Bytecode Structure:**

- **String table** - Deduplicated strings for efficient storage (on small programs adds overhead but this is a only a small tradeoff)
- **Required external functions** - External functions referenced by `.extern`
- **Function definitions** - Each function's operations and metadata, locals

---

## Notes

- Label addresses are resolved during assembly
- All numeric literals must use the `$` prefix
- All label references must use the `@` prefix