# Intermediate Code Generator - Three Address Code

An educational web application that shows how a compiler turns arithmetic assignments into Three Address Code (TAC). The user interface is written with HTML, CSS, and JavaScript, while the expression parser and TAC generator are implemented in C and compiled with GCC when the server starts.

## Features

- C-powered tokenization, validation, precedence handling, and TAC generation
- Support for `+`, `-`, `*`, `/`, `%`, parentheses, variables, and decimal numbers
- Dynamic token, operator, temporary, and instruction statistics
- Syntax-highlighted output with copy, download, and clear actions
- Error handling for malformed operators, missing parentheses, unsupported characters, and invalid assignments
- Responsive dark developer-tool interface with compiler pipeline visualization
- Ready-to-use example expressions and educational explanation sections

## Technologies and architecture

```text
TAC-Generator/
├── backend/server.js          Node HTTP server and C process bridge
├── compiler/tac_generator.c   Modular C TAC implementation
├── examples/examples.txt      Demo expressions
├── frontend/index.html        Application markup
├── frontend/style.css         Responsive visual system
├── frontend/script.js         UI state and API client
├── package.json
└── README.md
```

The Node server compiles `compiler/tac_generator.c` with GCC at startup. A `POST /api/generate` request sends the expression to the compiled executable through standard input. The executable writes TAC to standard output and validation errors to standard error. The browser never generates TAC itself.

## Requirements

- Node.js 18 or newer
- GCC available on `PATH` (MinGW GCC works on Windows)

## Run the project

From the project root:

```bash
npm start
```

Open `http://localhost:3000`. On Windows, `gcc` must be available in the terminal used to start Node. Set another port with `PORT=8080 npm start` or `$env:PORT=8080; npm start` in PowerShell.

## How TAC generation works

The C program separates the left side of the assignment, tokenizes the right side, and parses it with recursive-descent functions. `parsePrimary` handles values and parenthesized expressions, `parseTerm` handles multiplication, division, and modulus, and `parseExpression` handles addition and subtraction. Each reduction creates the next temporary (`t1`, `t2`, ...), followed by the final assignment.

## Sample input and output

### 1. `a = b + c * d`

```text
t1 = c * d
t2 = b + t1
a = t2
```

### 2. `x = (a + b) * c`

```text
t1 = a + b
t2 = t1 * c
x = t2
```

### 3. `result = a + b * c - d`

```text
t1 = b * c
t2 = a + t1
t3 = t2 - d
result = t3
```

### 4. `x = (a + b) * (c - d)`

```text
t1 = a + b
t2 = c - d
t3 = t1 * t2
x = t3
```

### 5. `total = price * quantity + tax`

```text
t1 = price * quantity
t2 = t1 + tax
total = t2
```

## Screenshots

Run the app locally and capture the workspace, generated TAC, and responsive mobile layout here.

## Future improvements

- Add unary minus and boolean/logical operators
- Add an AST visualization and step-through parser animation
- Add a WebAssembly build of the C engine for a serverless deployment
- Add saved sessions and downloadable compiler traces