# CIR Assembly (CAS) Language Reference

CIR Assembly (CAS) is a **function-based assembly language** where all code must be contained within functions. Global labels are not permitted.

---

## External Functions

External functions can be called using the `callx` instruction.
They always exist if registered, but you can also require one to be present at runtime.
```asm
.extern example
```

When a required function is not found, CIR will report an error:
```text
./build/cas ./example.cas -d examples/dl-imports/libexample.so
[INFO] Compiling: ./example.cas
[SUCCESS] Bytecode written to: example.bin (185 bytes)
[INFO] Executing program
[ERROR] Execution failed: Missing required external function: example
```

## Function Structure

Functions are declared using the `.fn` directive and terminated with `.end`:

```asm
.fn function_name
    ; function body
.end
```

**Key Points:**
- `.fn <name>` declares a function
- `.end` marks the end of the function
- All executable code must be inside a function
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

| Prefix | Type | Example | Description |
|--------|------|---------|-------------|
| `$` | Numeric literal | `$42`, `$3.14`, `$0xFF`, `$0b1010` | Integer or floating-point value (mandatory) |
| `"` | String literal | `"Hello"` | Text string |
| `@` | Label reference | `@loop_start` | Reference to a local label (mandatory) |
| `#` | Explicit identifier | `#my_var` | Force treat as identifier/string (optional) |
| `r` | Register | `r0`, `r1`, ... | Register reference |
| `'` | Character literal | `'a'`, `'\n'` | Single character (converted to integer) |

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
    mov comp(loop_start + 5), r0  ; Calculate address offset + in comp for labels you dont need @
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

## Instruction Format

### Single Operand Instructions

```asm
instruction operand
```

**Examples:**
```asm
push $42       ; Push value onto stack
pop r1         ; Pop value from stack into r1
inc r0         ; Increment register
jmp @label     ; Jump to label
call #function ; Call function
```

### Two Operand Instructions

```asm
instruction source, destination
```

**Examples:**
```asm
mov $100, r1      ; Move 100 into r1
iadd r0, r1       ; r0 = r0 + r1 (result in r0)
icmp r0, r1       ; Compare r0 and r1, set comparison flag

; moving one register into another will currently dont work because it moves a value into some register (TODO: update when finished)
mov r1, r0        ; Move r1 into r0 
```

---

## Instruction Set

### Data Movement

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `mov` | src, dest | Move value from src to dest |
| `push` | value | Push value onto stack |
| `pushr` | register | Push register value onto stack |
| `pop` | register | Pop value from stack into register |

### Integer Arithmetic

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `iadd` | reg1, reg2 | r0 = reg1 + reg2 |
| `isub` | reg1, reg2 | r0 = reg1 - reg2 |
| `imul` | reg1, reg2 | r0 = reg1 * reg2 |
| `idiv` | reg1, reg2 | r0 = reg1 / reg2 |
| `imod` | reg1, reg2 | r0 = reg1 % reg2 |
| `inc` | register | Increment register |
| `dec` | register | Decrement register |
| `neg` | register | r0 = -register |

### Floating-Point Arithmetic

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `fadd` | reg1, reg2 | r0 = reg1 + reg2 (float) |
| `fsub` | reg1, reg2 | r0 = reg1 - reg2 (float) |
| `fmul` | reg1, reg2 | r0 = reg1 * reg2 (float) |
| `fdiv` | reg1, reg2 | r0 = reg1 / reg2 (float) |
| `fcmp` | reg1, reg2 | Compare floats (TODO: not yet implemented) |

### Bitwise Operations

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `and` | reg1, reg2 | r0 = reg1 & reg2 |
| `or` | reg1, reg2 | r0 = reg1 \| reg2 |
| `xor` | reg1, reg2 | r0 = reg1 ^ reg2 |
| `not` | register | r0 = ~register |
| `shl` | reg1, reg2 | r0 = reg1 << reg2 |
| `shr` | reg1, reg2 | r0 = reg1 >> reg2 |

### Comparison and Control Flow

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `icmp` | reg1, reg2 | Compare reg1 == reg2, set comparison flag |
| `jmp` | @label | Unconditional jump to label |
| `je` | @label | Jump if equal (comparison flag true) |
| `jne` | @label | Jump if not equal (comparison flag false) |
| `jg` | @label | Jump if greater (TODO: not yet implemented) |
| `jl` | @label | Jump if less (TODO: not yet implemented) |
| `jge` | @label | Jump if greater or equal (TODO: not yet implemented) |
| `jle` | @label | Jump if less or equal (TODO: not yet implemented) |

### Function Calls

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `call` | function_name | Call CAS function |
| `callx` | function_name | Call external (C++) function |
| `ret` | - | Return from function |

### Type Conversion

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `cast` | type, register | Cast register to type ("int", "float", "ptr") |

### Local Variables

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `local.get` | local_id | r0 = locals[local_id] |
| `local.set` | local_id, register | locals[local_id] = register |

### Memory Operations

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `load` | dest, src | Load from memory (TODO: not yet implemented) |
| `store` | dest, src | Store to memory (TODO: not yet implemented) |

### Miscellaneous

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `halt` | - | Stop program execution |
| `nop` | - | No operation |

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
    iadd r0, r1      ; r0 = r0 + r1 = 30
    
loop_start:
    dec r0           ; r0--
    icmp r0, $0
    jne @loop_start  ; Loop while r0 != 0
    
    call #helper
    ret
.end

.fn helper
    push "Hello from helper!"
    callx #print
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

---

## Bytecode Format

Assembled programs are compiled to a compact bytecode format (`.cbc` files):

```cpp
Assembler asm;
asm.assemble_file("program.cas");
asm.write_bytecode("program.cbc");
```

**Bytecode Structure:**
- **String table** - Deduplicated strings for efficient storage
- **Function definitions** - Each function's operations and metadata
- **Local variable storage** - Variable slots per function
- **Binary representation** - Compact instruction encoding

---

## Notes

- The assembler provides helpful warnings for unadorned identifiers (can be disabled with `show_better_practice = false`)
- Label addresses are resolved during assembly
- All numeric literals must use the `$` prefix
- All label references must use the `@` prefix