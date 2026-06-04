# Compilador de Expresiones Aritméticas

Compilador completo escrito en C que procesa un subconjunto del lenguaje C (declaraciones de variables enteras y decimales, asignaciones y expresiones aritméticas). Implementa todas las fases clásicas de compilación: análisis léxico, sintáctico, semántico, generación de código intermedio (TAC) y generación de código ensamblador x86 (DOS/COM).

---

## Archivos del proyecto

| Archivo | Descripción |
|---|---|
| `Compilador_OA.c` | Versión final completa — todas las fases integradas |
| `Compilador.cpp` | Versión intermedia en C++ |
| `Analisis_Lexico.c` | Fase léxica aislada |
| `Analisis_Sintactico.c` | Fases léxica y sintáctica |
| `programa.asm` | Último archivo ASM generado por el compilador |
| `Tester_Comp_OA.txt` | Archivo de prueba de entrada |
 
---

## Lenguaje soportado

El compilador acepta programas con la siguiente estructura:

```
int x, y = 5;
float resultado;
x = 3 + y * 2;
resultado = x / 1.5;
```

### Tipos de datos
- `int` — entero
- `float` — decimal (punto flotante)

### Operadores
- Aritméticos: `+`, `-`, `*`, `/`
- Asignación: `=`

### Reglas
- Las declaraciones van antes que las sentencias.
- Se permiten múltiples variables por declaración separadas por coma.
- Se permiten números en notación decimal (`3.14`) y científica (`1.2E+5`).

---

## Fases del compilador

### 1. Análisis Léxico
Recorre el código carácter por carácter y genera tokens clasificados en:

| Token | Descripción | Ejemplo |
|---|---|---|
| `PR` | Palabra reservada | `int`, `float` |
| `ID` | Identificador | `x`, `resultado` |
| `NE` | Número entero | `42` |
| `ND` | Número decimal | `3.14` |
| `NX` | Número exponencial | `1.2E+5` |
| `OP` | Operador aritmético | `+`, `-`, `*`, `/` |
| `ASIG` | Asignación | `=` |
| `PYC` | Punto y coma | `;` |
| `PARI`/`PARD` | Paréntesis | `(`, `)` |
| `COMA` | Coma | `,` |

### 2. Análisis Sintáctico
Parser descendente recursivo que verifica la gramática:

```
PROG  → DECLS STMTS
DECLS → (int|float) ID [= EXP] {, ID [= EXP]} ; DECLS | ε
STMTS → ID = EXP ; STMTS | ε
EXP   → TERM { (+|-) TERM }
TERM  → FACT { (*|/) FACT }
FACT  → (EXP) | ID | NE | ND | NX
```

Construye un **árbol sintáctico** para cada sentencia de asignación.

### 3. Análisis Semántico
Detecta los siguientes errores en tiempo de compilación:

- Variable no declarada
- Variable redeclarada
- Asignación de valor decimal a variable entera
- División entre cero (con literales)
- Variable usada antes de inicializarse (solicita valor al usuario)

### 4. Código Intermedio (TAC)
Genera instrucciones de tres direcciones. Ejemplo:

```
t1 = y * 200
t2 = 300 + t1
x = t2
```

Los valores decimales se escalan por 100 para operar con aritmética entera.

### 5. Generación de Código ASM
Produce un archivo `programa.asm` compatible con el formato COM de DOS (TASM/MASM/DOSBox). Incluye:
- Declaración de variables y temporales en memoria
- Instrucciones MOV, ADD, SUB, IMUL, IDIV
- Procedimientos `print_int` y `print_fixed` para imprimir resultados
- Impresión automática de todas las variables al final

---

## Compilación y uso

### Compilar el compilador
```bash
gcc Compilador_OA.c -o compilador
```

### Ejecutar
```bash
./compilador
```
El programa pedirá el nombre del archivo de entrada:
```
Ingrese nombre del archivo (.txt): Tester_Comp_OA.txt
```

### Salida esperada
1. Contenido leído del archivo
2. Tabla de símbolos (lexema, token, tipo, regla)
3. Confirmación de análisis sintáctico y semántico correcto
4. Código intermedio TAC
5. Archivo `programa.asm` generado

---

## Ejemplo

**Entrada (`Tester_Comp_OA.txt`):**
```
int x;
float y;
x = 3 + 2;
y = x / 1.5;
```

**TAC generado:**
```
t1 = 300 + 200
x = t1
t2 = x * 100
t3 = 150 / 1      (escala aplicada)
y = t3
```

**ASM generado (`programa.asm`):** archivo listo para ensamblar con TASM o ejecutar en DOSBox.

---

## Limitaciones

- No soporta estructuras de control (`if`, `while`, `for`).
- No soporta funciones ni arreglos.
- El código ASM generado es para arquitectura x86 de 16 bits (DOS COM).
- Máximo 200 variables declaradas y 1000 tokens por programa.
