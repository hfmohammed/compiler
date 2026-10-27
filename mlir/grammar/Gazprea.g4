grammar Gazprea;

@lexer::members {
  // A leading-dot real like `.5` may not directly follow an identifier or ')'
  // (that dot is tuple access, as in `t.1`).
  bool dotRealOk() {
    size_t c = _input->LA(-1);
    bool word = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_' || c == ')';
    return !word;
  }
}

program : topLevel* EOF ;

topLevel : func | typeAliasDecl ;

typeAliasDecl : TYPEALIAS type ID ';' ;

func
  : kind=(FUNCTION | PROCEDURE) name=ID '(' (param (',' param)*)? ')'
    (RETURNS type)? ( '=' expr ';' | '{' stmt* '}' | ';' )
  ;

param : qual=(CONST | VAR)? type ID? ;

type
  : KW_INTEGER | KW_REAL | KW_BOOLEAN | KW_CHARACTER
  | KW_TUPLE '(' type (',' type)* ')'
  | ID
  ;

stmt
  : '{' stmt* '}'                                      # BlockStmt
  | typeAliasDecl                                      # TypeAliasStmt
  | qual=(CONST | VAR) type? ID ('=' expr)? ';'        # QualDecl
  | type ID ('=' expr)? ';'                            # TypedDecl
  | IF '(' expr ')' thenS=stmt (ELSE elseS=stmt)?      # IfStmt
  | LOOP WHILE '(' expr ')' stmt                       # WhileLoop
  | LOOP ID IN expr '..' expr stmt                     # RangeLoop
  | LOOP stmt (WHILE '(' expr ')' ';')?                # Loop
  | BREAK ';'                                          # BreakStmt
  | CONTINUE ';'                                       # ContinueStmt
  | RETURN expr? ';'                                   # ReturnStmt
  | CALL ID '(' (expr (',' expr)*)? ')' ';'            # CallStmt
  | ID '=' expr ';'                                    # AssignStmt
  | ID '.' INT_LIT '=' expr ';'                        # TupleAssignStmt
  | ID (',' ID)+ '=' expr ';'                          # UnpackStmt
  | ID ('.' INT_LIT)? '<-' STD_INPUT ';'               # InputStmt
  | expr '->' STD_OUTPUT ';'                           # OutputStmt
  ;

// Highest precedence first. '^' binds tighter than the unary operators.
expr
  : '(' expr ')'                                       # ParenExpr
  | '(' expr (',' expr)+ ')'                           # TupleLit
  | ID '.' INT_LIT                                     # TupleAccess
  | ID '(' (expr (',' expr)*)? ')'                     # CallExpr
  | AS '<' type '>' '(' expr ')'                       # CastExpr
  | <assoc=right> expr '^' expr                        # PowExpr
  | op=('-' | '+' | NOT) expr                          # UnaryExpr
  | expr op=('*' | '/' | '%') expr                     # MulExpr
  | expr op=('+' | '-') expr                           # AddExpr
  | expr op=('<' | '>' | '<=' | '>=') expr             # RelExpr
  | expr op=('==' | '!=') expr                         # EqExpr
  | expr AND expr                                      # AndExpr
  | expr op=(OR | XOR) expr                            # OrExpr
  | INT_LIT                                            # IntLit
  | REAL_LIT                                           # RealLit
  | CHAR_LIT                                           # CharLit
  | STRING_LIT                                         # StringLit
  | STREAM_STATE '(' STD_INPUT ')'                     # StreamStateExpr
  | (TRUE | FALSE)                                     # BoolLit
  | ID                                                 # VarRef
  ;

FUNCTION  : 'function' ;
PROCEDURE : 'procedure' ;
RETURNS   : 'returns' ;
RETURN    : 'return' ;
VAR       : 'var' ;
CONST     : 'const' ;
IF        : 'if' ;
ELSE      : 'else' ;
LOOP      : 'loop' ;
WHILE     : 'while' ;
BREAK     : 'break' ;
CONTINUE  : 'continue' ;
CALL      : 'call' ;
AS        : 'as' ;
TYPEALIAS : 'typealias' ;
TRUE      : 'true' ;
FALSE     : 'false' ;
NOT       : 'not' ;
AND       : 'and' ;
OR        : 'or' ;
XOR       : 'xor' ;
KW_INTEGER   : 'integer' ;
KW_REAL      : 'real' ;
KW_BOOLEAN   : 'boolean' ;
KW_CHARACTER : 'character' ;
KW_TUPLE     : 'tuple' ;
IN         : 'in' ;
STREAM_STATE : 'stream_state' ;
STD_INPUT  : 'std_input' ;
STD_OUTPUT : 'std_output' ;

ID : [a-zA-Z_] [a-zA-Z_0-9]* ;

REAL_LIT
  : DIGIT+ '.' {_input->LA(1) != '.'}? DIGIT* EXP?
  | {dotRealOk()}? '.' DIGIT+ EXP?
  | DIGIT+ EXP
  ;
INT_LIT : DIGIT+ ;
// Escapes are validated when the AST is built (bad ones are a LiteralError).
CHAR_LIT   : '\'' ( ~['\\\r\n] | '\\' ~[\r\n] ~['\r\n]* ) '\'' ;
STRING_LIT : '"' ( ~["\\\r\n] | '\\' ~[\r\n] )* '"' ;

fragment DIGIT : [0-9] ;
fragment EXP   : [eE] [+-]? DIGIT+ ;

LINE_COMMENT  : '//' ~[\r\n]* -> skip ;
BLOCK_COMMENT : '/*' .*? '*/' -> skip ;
WS : [ \t\r\n]+ -> skip ;
