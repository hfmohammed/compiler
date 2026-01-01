# Gazprea Language Grammar

This document describes the grammar of the Gazprea style language used by the compiler. The grammar is written in an EBNF inspired notation. Some constructs are specified for completeness but are not yet implemented in code generation. These are explicitly marked with **[not yet implemented]**.

---

## Lexical Elements

```
identifier    ::= letter { letter | digit | '_' }
integer_lit   ::= digit { digit }
string_lit    ::= '"' { character } '"'
boolean_lit   ::= true | false
```

Keywords:

```
const var function procedure returns struct typealias
if else loop while break continue return call
true false not and or
```

---

## Program Structure

```
program ::= { program_element }

program_element ::= declaration
                  | struct_declaration
                  | function_declaration
                  | procedure_declaration
                  | statement
                  | typealias_declaration
```

---

## Type System

```
type ::= 'integer'
       | 'real'                       [not yet implemented]
       | 'boolean'
       | 'string'
       | identifier                  // struct or alias
       | tuple_type                  [partially implemented]
       | array_type                  [partially implemented]


tuple_type ::= 'tuple' '(' type { ',' type } ')'

array_type ::= type '[' dimension ']'

dimension ::= integer_lit | '*'
```

---

## Type Alias

```
typealias_declaration ::= 'typealias' identifier identifier ';'
```

---

## Declarations

```
declaration ::= 'const' type identifier '=' expression ';'
              | 'var' type identifier ';'
              | 'var' type identifier '=' expression ';'
```

---

## Struct Declaration

```
struct_declaration ::= 'struct' identifier '(' field_list ')' ';'

field_list ::= field { ',' field }

field ::= type identifier
```

Struct field access:

```
struct_access ::= identifier '.' identifier   [not yet implemented]
```

---

## Expressions

```
expression ::= assignment

assignment ::= logical_or
             | identifier '=' expression

logical_or ::= logical_and { 'or' logical_and }

logical_and ::= equality { 'and' equality }

equality ::= relational { ('==' | '!=') relational }

relational ::= additive { ('<' | '>' | '<=' | '>=') additive }

additive ::= multiplicative { ('+' | '-') multiplicative }

multiplicative ::= unary { ('*' | '/') unary }

unary ::= ('-' | 'not' | '+') unary
        | primary
```

---

## Primary Expressions

```
primary ::= integer_lit
          | string_lit
          | boolean_lit
          | identifier
          | function_call
          | '(' expression ')'
          | struct_access            [not yet implemented]
          | tuple_expression         [not yet implemented]
          | array_access             [not yet implemented]
```

---

## Function Call

```
function_call ::= identifier '(' [ argument_list ] ')'

argument_list ::= expression { ',' expression }
```

---

## Statements

```
statement ::= declaration
            | assignment ';'
            | function_call ';'
            | block
            | if_statement
            | loop_statement
            | return_statement
            | break_statement
            | continue_statement
            | stream_statement       [not yet implemented]
            | tuple_assignment       [not yet implemented]
```

---

## Block

```
block ::= '{' { statement } '}'
```

---

## If Statement

```
if_statement ::= 'if' '(' expression ')' statement
                 { 'else if' '(' expression ')' statement }
                 [ 'else' statement ]
```

---

## Loop Statements

```
loop_statement ::= 'loop' 'while' '(' expression ')' statement
                 | 'loop' statement 'while' expression ';'
                 | 'loop' statement
```

---

## Function Declaration

```
function_declaration ::= 'function' identifier '(' parameter_list ')'
                         'returns' type
                         ( '=' expression ';' | block )
```

---

## Procedure Declaration

```
procedure_declaration ::= 'procedure' identifier '(' parameter_list ')'
                          block
```

---

## Parameters

```
parameter_list ::= [ parameter { ',' parameter } ]

parameter ::= [ 'var' ] type identifier
```

---

## Return Statement

```
return_statement ::= 'return' expression ';'
```

---

## Tuple Assignment

```
tuple_assignment ::= identifier_list '=' tuple_expression   [not yet implemented]

identifier_list ::= identifier { ',' identifier }
```

---

## Streams

```
stream_statement ::= expression '->' identifier              [not yet implemented]
                   | identifier '<-' identifier              [not yet implemented]
```

---

## Notes

* Functions return values and procedures do not
* Parameters declared with `var` are mutable
* Stack based evaluation is used for expressions
* Nested function calls are supported
* Constructs marked as not yet implemented exist in the grammar but are not fully supported by code generation yet
