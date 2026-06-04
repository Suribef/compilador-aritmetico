#include <stdio.h>// Para leer archivos y escribir en pantalla
#include <stdlib.h>// Para manejar memoria y terminar el programa si hay errores
#include <string.h>// Para comparar y copiar palabras
#include <ctype.h>// Para saber si un caracter es letra o número

#define MAX_ENTRADA 10000 // Tamaño máximo del archivo que podemos leer
#define SCALE 100 // Truco matemático: los decimales se multiplican por 100 para usarlos como enteros

/* ================= ALFABETOS ================= */

char alfabetoLetras[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";// Letras permitidas para nombres de variables
char alfabetoOps[]    = "+-*/";// Operaciones matemáticas válidas
char alfabetoNums[]   = "0123456789";// Números permitidas

/* ================= PALABRAS RESERVADAS ================= */

const char *palabrasReservadas[] = {"int","float",NULL};// Palabras que el usuario NO puede usar como nombres

/* ================= PROTOTIPOS NUMERICOS ================= */

int esEntero(char *lex);
int esDecimal(char *lex);
int esExponencial(char *lex);
int expresionDecimal = 0;
 

/* ================= ESTRUCTURA TOKEN ================= */

// Estructura para guardar cada palabra encontrada (Token)
typedef struct {
    char lexema[100];// La palabra original (ej: "x")
    char token[10];// El tipo (ej: "ID" para identificador)
    char tipoDato[10];// "ent" (entero) o "dec" (decimal)
    char regla[100];// Nota descriptiva
} Simbolo;

Simbolo tabla[1000];// Aquí guardamos todas las palabras del código leído
int tablaIndex = 0;
int lineaActual = 1;

/* ================= TABLA SEMANTICA ================= */

// Estructura para variables declaradas (Tabla Semántica)

typedef struct{
    char nombre[50];
    char tipo[10];
    int inicializada;// Para saber si ya se le asignó un valor (evita errores de basura)
} Variable;

Variable tablaVars[200];

int varIndex = 0;

/* ================= TAC ================= */

char TAC[1000][100];// Guarda las instrucciones intermedias tipo: t1 = a + b
int tacIndex = 0;
int tempIndex = 1;

// Crea nombres automáticos para variables temporales (t1, t2, t3...)
char* nuevaTemp(){
char *buffer = (char*)malloc(20);

    if(!buffer){

        printf("Error: sin memoria para temporal\n");
        exit(1);

    }

    sprintf(buffer,"t%d",tempIndex++);
    return buffer;
}

void generarTAC(char *res, char *a, char op, char *b){

    if(tacIndex >= 1000){
        printf("Error: TAC overflow\n");
        exit(1);
    }



sprintf(TAC[tacIndex++],"%s = %s %c %s",res,a,op,b);
}

/* ================= ARBOL ================= */

    typedef struct Nodo{
        char valor[50];
        struct Nodo *izq;// Hijo izquierdo
        struct Nodo *der;// Hijo derecho
    }Nodo;

    // Ejemplo: Para "3 + 4", el nodo padre es "+" y sus hijos son "3" y "4".
    Nodo* arboles[200];
    int arbolIndex = 0;

    // Función para crear un nuevo nodo del árbol
Nodo* nuevoNodo(char *v){
Nodo *n = (Nodo*)malloc(sizeof(Nodo));

    if(!n){
        printf("Error: sin memoria\n");
        exit(1);
    }

    strcpy(n->valor,v);
    n->izq=NULL;
    n->der=NULL;
    return n;
}

// Función para imprimir el árbol (para depuración)

// Convierte un número en string a su valor entero escalado (ej: "3.14" -> 314)
long convertirNumero(char *lex){
    if(esEntero(lex))
        return atol(lex)*SCALE;
    if(esDecimal(lex))
        return (long)(atof(lex)*SCALE);
    if(esExponencial(lex))
        return (long)(atof(lex)*SCALE);
    return 0;
}

//=======================PROTOTIPOS==========================/

// Prototipos de funciones para el análisis sintáctico (parser)

void PROG();// Función principal del análisis sintáctico, que verifica la estructura general del programa
void DECLS();// Analiza las declaraciones de variables al inicio del programa
void STMTS();// Analiza las sentencias de asignación después de las declaraciones
Nodo* EXP(char *place);// Analiza expresiones con suma y resta
Nodo* TERM(char *place);// Analiza términos con multiplicación y división
Nodo* FACT(char *place);// Analiza factores que pueden ser números, variables o expresiones entre paréntesis
char* obtenerTipo(char *nombre);// Función para obtener el tipo de una variable a partir de su nombre, buscando en la tabla semántica
char* obtenerTipoExpresion(char *exp);// Función para determinar el tipo de una expresión (entero o decimal) a partir de su contenido (números, variables, temporales)

/* ================= FUNCIONES SEMANTICAS ================= */

// Función para declarar una variable: verifica redeclaración y guarda en la tabla semántica

void declararVariable(char *nombre, char *tipo){

// Verificar que no se haya declarado antes

for(int i=0;i<varIndex;i++){

    if(strcmp(tablaVars[i].nombre,nombre)==0){
        printf("\nError semantico: variable redeclarada -> %s\n",nombre);
        exit(1);
    }

}   // Guardar en la tabla semántica

    if(varIndex >= 200){
        printf("Error: demasiadas variables\n");
        exit(1);
    }

    strcpy(tablaVars[varIndex].nombre,nombre);// Guardar el nombre de la variable
    strcpy(tablaVars[varIndex].tipo,tipo);// Guardar el tipo (ent o dec)
    varIndex++;
}

// Función para obtener el tipo de una variable (ent o dec) a partir de su nombre
char* obtenerTipo(char *nombre){
    for(int i=0;i<varIndex;i++)
        if(strcmp(tablaVars[i].nombre,nombre)==0)
            return tablaVars[i].tipo;
    return NULL;
}

// Función para revisar el TAC y pedir valores de variables que se usan sin inicializar pero no aparecen como destino en el TAC (ej: variables usadas en expresiones pero nunca asignadas)
void pedirVariablesNoInicializadas(){
    // Para cada variable declarada, revisar si se usa antes de ser asignada: buscamos si aparece en el lado derecho de alguna instrucción antes de aparecer en el lado izquierdo de una asignación
    for(int i=0;i<varIndex;i++){
        int usadaAntesAsignacion = 0;
        int asignada = 0;
        // Revisar el TAC para ver si la variable se usa antes de ser asignada: buscamos si aparece en el lado derecho de alguna instrucción antes de aparecer en el lado izquierdo de una asignación
        for(int j=0;j<tacIndex;j++){
            char res[50], op1[50], op2[50];
            char operador;
            if(sscanf(TAC[j],"%s = %s %c %s",res,op1,&operador,op2)==4){
                if(strcmp(op1,tablaVars[i].nombre)==0 ||
                   strcmp(op2,tablaVars[i].nombre)==0){
                    if(!asignada)
                        usadaAntesAsignacion = 1;
                }
                if(strcmp(res,tablaVars[i].nombre)==0)
                    asignada = 1;
            }

            else if(sscanf(TAC[j],"%s = %s",res,op1)==2){
                if(strcmp(op1,tablaVars[i].nombre)==0 && !asignada)
                    usadaAntesAsignacion = 1;
                if(strcmp(res,tablaVars[i].nombre)==0)
                    asignada = 1;
            }
        }
        // Si la variable se usa antes de ser asignada, pedimos un valor al usuario para evitar errores de basura en tiempo de ejecución. 
        //Luego generamos una instrucción TAC para asignar ese valor a la variable al inicio del programa.
        if(usadaAntesAsignacion){
            char input[50];
            printf("\nLa variable '%s' se usa antes de inicializarse.\n",tablaVars[i].nombre);
            printf("Ingrese valor: ");
            scanf("%s",input);

            long valor = convertirNumero(input);
            for(int j=tacIndex; j>0; j--)
                strcpy(TAC[j],TAC[j-1]);
            sprintf(TAC[0],"%s = %ld",tablaVars[i].nombre,valor);
            tacIndex++;
            tablaVars[i].inicializada = 1;
        }
    }
}

// Función para actualizar el tipo de una variable en la tabla de símbolos después de declararla 
//(ej: después de procesar "int x;", actualizamos el token "ID" con lexema "x" para que tenga tipo "ent")
void actualizarTipoTabla(char *nombre, char *tipo){

    for(int i=0;i<tablaIndex;i++){
        if(strcmp(tabla[i].lexema,nombre)==0 &&
        strcmp(tabla[i].token,"ID")==0){
        strcpy(tabla[i].tipoDato,tipo);
        }
    }
}

// Función para combinar tipos en operaciones: si alguno es decimal, el resultado es decimal
char* combinarTipos(char *a, char *b){
if(strcmp(a,"dec")==0 || strcmp(b,"dec")==0)
    return "dec";
    return "ent";
}

// Función para determinar el tipo de una expresión a partir de su contenido: si contiene un número decimal, una variable decimal o un temporal, se considera decimal; si solo tiene enteros y variables enteras, se considera entero
char* obtenerTipoExpresion(char *exp){
    /* si es numero entero */
    if(esEntero(exp))
        return "ent";
    /* si es numero decimal */
    if(esDecimal(exp) || esExponencial(exp))
        return "dec";
    /* si es variable */
    char *tipo = obtenerTipo(exp);
    if(tipo != NULL)
        return tipo;
    /* si es temporal (t1, t2, etc) */
    if(exp[0]=='t')
        return "dec";
    return "ent";
}

/* ================= FUNCIONES DE UTILIDAD ================= */
// Verifica si un caracter pertenece a un alfabeto dado
int pertenece(char c, char *alfabeto){
    for(int i=0; alfabeto[i]; i++)
        if(c == alfabeto[i])
            return 1;
    return 0;
}

// Funciones para verificar si un caracter es letra, número u operador

int esLetra(char c){ return pertenece(c, alfabetoLetras); }// Letras para nombres de variables
int esNumero(char c){ return pertenece(c, alfabetoNums); }// Dígitos para números
int esOperador(char c){ return pertenece(c, alfabetoOps); }// Operadores matemáticos

// Verifica si una palabra es una palabra reservada ("int" o "float")

int esReservada(char *lexema){
    for(int i=0; palabrasReservadas[i]!=NULL; i++)
        if(strcmp(lexema, palabrasReservadas[i])==0)
            return 1;
    return 0;
}

/* ================= INSERTAR TABLA ================= */
// Inserta un nuevo token en la tabla de símbolos con su lexema, tipo y regla asociada
void insertar(const char *lexema, const char *token, const char *tipo, const char *regla){

    // Verificar que no se exceda el tamaño de la tabla
    if(tablaIndex >= 1000){
        printf("ERROR: tabla de simbolos llena\n");
        exit(1);
    }

    // Guardar el token en la tabla
    strcpy(tabla[tablaIndex].lexema, lexema);// La palabra original
    strcpy(tabla[tablaIndex].token, token);// El tipo de token (ej: "ID", "NE", "OP", etc.)
    strcpy(tabla[tablaIndex].tipoDato, tipo);// "ent" para enteros, "dec" para decimales, "-" si no aplica
    strcpy(tabla[tablaIndex].regla, regla);// Nota descriptiva o regla gramatical asociada (ej: "Suma", "Multiplicacion", "identificador", etc.)
    tablaIndex++;
}

/* ================= ERROR LEXICO ================= */
// Si se encuentra un símbolo que no pertenece al lenguaje, se muestra un error léxico con la línea y el símbolo problemático
void errorLexico(const char *lex){
    // Mostrar un mensaje de error con la línea actual y el símbolo no válido
    printf("\nERROR LEXICO (linea %d): simbolo no valido -> %s\n",
        lineaActual, lex);
    exit(1);
}

/* ================= RECONOCIMIENTO NUMEROS ================= */
// Verifica si una palabra es un número entero, decimal o exponencial
int esEntero(char *lex){
int i=0;
int digitos=0;
    if(strlen(lex) > 10){
        printf("\nError: entero demasiado grande -> %s\n",lex);
        exit(1);
    }

// Permitir un signo opcional al inicio
if(lex[i]=='+' || lex[i]=='-')
    i++;

    for(; lex[i]; i++){
        if(!esNumero(lex[i]))
            return 0;
        digitos++;
    }
    return digitos>0;
}

// Verifica si es un número decimal (ej: 3.14)
int esDecimal(char *lex){
int punto = 0;
int i=0;
int digitosDespues = 0;

// Para evitar overflow al convertir a entero escalado, limitamos la cantidad de dígitos permitidos (ej: 123456789012345.67 tiene 15 dígitos antes del punto, lo cual es seguro para convertir a un entero escalado sin overflow)
    if(strlen(lex) > 15){
        printf("\nError: decimal demasiado grande -> %s\n",lex);
        exit(1);
    }
    // Permitir un signo opcional al inicio
    if(lex[i]=='.'){// Si el primer caracter es un punto, debe haber dígitos después
        punto = 1;
        i++;
        while(esNumero(lex[i])){
            digitosDespues++;
            i++;
        }
        if(digitosDespues == 0)
            return 0;
    }

    if(lex[i]=='+' || lex[i]=='-') i++;// Permitir un signo opcional después del punto (ej: 3.-14)
        for(; lex[i]; i++){
            if(lex[i]=='.'){
                if(punto) return 0;
                punto = 1;
            }
            else if(!esNumero(lex[i]))
                return 0;
        }
    return punto;
}

// Verifica si es notación científica (ej: 1.2E+5)
int esExponencial(char *lex){
int i=0;
    if(lex[i]=='+' || lex[i]=='-') i++;

    while(esNumero(lex[i])) i++;
        if(lex[i]=='.'){
            i++;
            while(esNumero(lex[i])) i++;
        }
    if(lex[i]=='E' || lex[i]=='e'){
        i++;
        if(lex[i]=='+' || lex[i]=='-') i++;
        if(!esNumero(lex[i])) return 0;
        while(esNumero(lex[i])) i++;
        if(lex[i]=='\0') return 1;
    }
    return 0;
}

/* ================= SIGNO NUMERICO ================= */
// Verifica si un '+' o '-' es parte de un número (ej: 3 + -2) o es un operador (ej: 3 - 2)
int esSignoNumero(const char *codigo, int i){
    if(codigo[i] != '+' && codigo[i] != '-')
        return 0;
    int j = i - 1;

    while(j >= 0 && (codigo[j]==' ' || codigo[j]=='\t' || codigo[j]=='\n'))
        j--;

        if(j < 0)
        return 1;

        char prev = codigo[j];
    return prev=='(' || prev=='=' || esOperador(prev) || prev=='e' || prev=='E';
}

/* ================= IDENTIFICADOR ================= */
// Verifica si una palabra es un identificador válido (empieza con letra, seguido de letras o números)
int esIdentificador(char *lex){
    if(!esLetra(lex[0])) return 0;
    for(int i=1; lex[i]; i++)
        if(!esLetra(lex[i]) && !esNumero(lex[i]))
            return 0;
    return 1;
}

/* ================= ANALIZADOR LEXICO ================= */
// Función principal del Lexer: clasifica la palabra y la almacena en la 'TS'
void analizarLexema(char *lex){
    if(strlen(lex)==0) return;
    if(esReservada(lex)){
        insertar(lex,"PR","-","-");
        return;
    }
    if(esExponencial(lex)){
        insertar(lex,"NX","dec","-");
        return;
    }
    if(esEntero(lex)){
        insertar(lex,"NE","ent","-");
        return;
    }
    if(esDecimal(lex)){
        insertar(lex,"ND","dec","-");
        return;
    }
    if(esIdentificador(lex)){
        insertar(lex,"ID","-","identificador");
        return;
    }
    errorLexico(lex);
}

/* ================= IMPRESION TABLA ================= */
// Imprime la tabla de símbolos con sus lexemas, tokens, tipos y reglas asociadas
void imprimirTabla(){
    printf("\nLEXEMA\tTOKEN\tTIPO\tREGLA\n");
    printf("-------------------------------------------------\n");

    for(int i=0;i<tablaIndex;i++){
        printf("%s\t%s\t%s\t%s\n",
        tabla[i].lexema,
        tabla[i].token,
        tabla[i].tipoDato,
        tabla[i].regla);
    }
}

/* ================= LECTURA ================= */
// Verifica si un caracter es un símbolo válido del lenguaje (letra, número, operador, paréntesis, etc.)
int esSimboloLenguaje(char c){
return esLetra(c) || esNumero(c) ||
       esOperador(c) ||
       c=='.' ||
       c=='(' || c==')' ||
       c=='=' || c==';' ||
       c==',' ||
       c==' ' || c=='\n' || c=='\t';

}

// Función principal del Lexer: lee el código carácter por carácter, forma palabras (lexemas) y las clasifica
void analizarCadena(const char *codigo){
int i = 0;
char buffer[100];
int idx = 0;
char c;

    while((c = codigo[i]) != '\0'){
        if(c=='\n')
            lineaActual++;
        // Si el caracter es parte de un identificador, número o operador, lo agregamos al buffer para formar el lexema
        if(esLetra(c) || esNumero(c) || c=='.' || esSignoNumero(codigo,i))
        {
            if(idx >= 99){
                printf("\nError: lexema demasiado largo\n");
                exit(1);
            }
            buffer[idx++] = c;
        }
        // Si encontramos un espacio, tabulador, salto de línea o símbolo del lenguaje, terminamos el lexema actual y lo analizamos
        else{
            buffer[idx] = '\0';
            analizarLexema(buffer);
            idx = 0;
            // Si el caracter es un espacio, tabulador o salto de línea, simplemente lo ignoramos
            if(c==' ' || c=='\t' || c=='\n'){
                i++;
                continue;
            }
            // Si el caracter es un símbolo del lenguaje (ej: +, -, *, /, (, ), =, ;, ,), lo clasificamos directamente
            if(!esSimboloLenguaje(c)){
                char error[2] = {c,'\0'};
                errorLexico(error);
            }

            char lex[2] = {c,'\0'};
            // Clasificar operadores y símbolos directamente sin necesidad de buffer
            if(esOperador(c)){
                char regla[100];
                switch(c){
                    case '+': strcpy(regla,"Suma"); break;

                    case '-': strcpy(regla,"Resta"); break;

                    case '*': strcpy(regla,"Multiplicacion"); break;

                    case '/': strcpy(regla,"Division"); break;

                    default: strcpy(regla,"Operador");
                }
                insertar(lex,"OP","-",regla);
            }

            switch(c){
                case '=': insertar(lex,"ASIG","-","MOV"); break;
                case '(': insertar(lex,"PARI","-","-"); break;
                case ')': insertar(lex,"PARD","-","-"); break;
                case ';': insertar(lex,"PYC","-","-"); break;
                case ',': insertar(lex,"COMA","-","-"); break;
            }

        }
        i++;
    }
    buffer[idx] = '\0';
    analizarLexema(buffer);
}

/* ================= ANALISIS SINTACTICO ================= */
// Variable global para llevar la posición actual en la tabla de símbolos durante el análisis sintáctico
int pos = 0;
int enLadoIzquierdo = 0;
void errorSintactico(const char *msg){// Muestra un mensaje de error sintáctico con la posición y el token problemático
    printf("\nError sintactico: %s\n", msg);

    if(pos < tablaIndex)
        printf("Cerca de: %s\n", tabla[pos].lexema);
    exit(1);
}

// Verifica que el token actual en la tabla de símbolos coincida con el esperado, y avanza la posición
int match(const char *token){
    if(pos < tablaIndex && strcmp(tabla[pos].token, token)==0){
        pos++;
        return 1;
    }
    return 0;
}

// Función principal del análisis sintáctico: verifica que el programa siga la estructura correcta de declaraciones seguidas de sentencias
void PROG(){
    DECLS();
    STMTS();
    pedirVariablesNoInicializadas();   // ← AQUÍ
    if(pos != tablaIndex)
        errorSintactico("Codigo extra despues del programa");
}

/* ================= DECLARACIONES ================= */
// Revisa declaraciones como: int x, y = 10;
void DECLS(){

// Mientras encontremos declaraciones de variables (int o float), las procesamos
while(pos < tablaIndex &&
     (strcmp(tabla[pos].lexema,"int")==0 ||
      strcmp(tabla[pos].lexema,"float")==0)){

        char tipoActual[10];
        // Determinar el tipo de la variable (entero o decimal) según la palabra reservada encontrada
        if(strcmp(tabla[pos].lexema,"int")==0)
            strcpy(tipoActual,"ent");
        else
            strcpy(tipoActual,"dec");
        pos++;

        // Esperar un identificador después de la palabra reservada
        if(!match("ID"))
            errorSintactico("Se esperaba identificador");

            
        char nombreVar[50];
        strcpy(nombreVar,tabla[pos-1].lexema);
        declararVariable(nombreVar,tipoActual);
        actualizarTipoTabla(nombreVar,tipoActual);
        tablaVars[varIndex-1].inicializada = 0;

        // Si después del identificador hay un '=', significa que la variable se declara e inicializa en la misma línea (ej: int x = 5;)
        if(match("ASIG")){ 
            char res[50];
        expresionDecimal = 0;

        Nodo *arbolExp = EXP(res);
        //
        if(strcmp(tipoActual,"ent")==0 && expresionDecimal){
            printf("\nError semantico: no se puede asignar decimal a entero -> %s\n",nombreVar);
            exit(1);
        }

        sprintf(TAC[tacIndex++],"%s = %s",nombreVar,res);// Guardar el resultado de la expresión en el TAC
        Nodo *asig = nuevoNodo("=");// Crear un nodo de asignación para el árbol sintáctico
                Nodo *varNodo = nuevoNodo(nombreVar);// Nodo para la variable en el lado izquierdo de la asignación
                asig->izq = varNodo;
                asig->der = arbolExp;
                arboles[arbolIndex++] = asig;
                tablaVars[varIndex-1].inicializada = 1;
            }

        // Si después del primer identificador hay una coma, significa que se están declarando varias variables del mismo tipo en la misma línea (ej: int x, y = 10;)
        while(match("COMA")){
            // Esperar otro identificador después de la coma
            if(!match("ID"))
                errorSintactico("Se esperaba identificador");
            
            // Declarar la nueva variable y agregarla a la tabla semántica
            strcpy(nombreVar,tabla[pos-1].lexema);
            declararVariable(nombreVar,tipoActual);
            actualizarTipoTabla(nombreVar,tipoActual); 
            tablaVars[varIndex-1].inicializada = 0;

            // Si esta variable también se inicializa en la misma línea (ej: int x, y = 10;), procesar la asignación
            if(match("ASIG")){
                char res[50];

                Nodo *arbolExp = EXP(res);
                
                sprintf(TAC[tacIndex++],"%s = %s",nombreVar,res);
                
                Nodo *asig = nuevoNodo("=");
                Nodo *varNodo = nuevoNodo(nombreVar);
                
                asig->izq = varNodo;
                asig->der = arbolExp;
                
                arboles[arbolIndex++] = asig;

                /* MARCAR COMO INICIALIZADA */
                tablaVars[varIndex-1].inicializada = 1;

            }
        }
        if(!match("PYC"))
            errorSintactico("Se esperaba ;");
    }

}

/* ================= FACTOR ================= */
// Analiza operaciones matemáticas respetando la jerarquía (paréntesis, luego * y /, luego + y -)
// Números, variables o paréntesis
Nodo* FACT(char *place){
    // Si el factor es una expresión entre paréntesis, la analizamos recursivamente
    if(match("PARI")){
        char temp[50];
        Nodo *n = EXP(temp);
        strcpy(place,temp);
        if(!match("PARD"))
            errorSintactico("Se esperaba )");
    return n;
}

    // Si el factor es un identificador, verificamos que esté declarado y lo devolvemos como nodo del árbol
    if(match("ID")){
        strcpy(place,tabla[pos-1].lexema);
        
        int encontrada = -1;
        // Verificar que la variable esté declarada en la tabla semántica
        for(int i=0;i<varIndex;i++)
            if(strcmp(tablaVars[i].nombre,place)==0)
                encontrada=i;

        // Si no se encuentra la variable, es un error semántico
        if(encontrada==-1){
            printf("\nError semantico: variable no declarada -> %s\n",place);
            exit(1);
        }
        return nuevoNodo(place);
    }

    // Si el factor es un número literal (entero o decimal), lo convertimos a su valor escalado y lo devolvemos como nodo del árbol

    if(match("NE") || match("ND") || match("NX")){
        if(strcmp(tabla[pos-1].token,"ND")==0 ||
            strcmp(tabla[pos-1].token,"NX")==0)
        
            expresionDecimal = 1;
        long valor = convertirNumero(tabla[pos-1].lexema);// Convertir el número a su valor entero escalado (ej: 3.14 -> 314)
        sprintf(place,"%ld",valor);// Guardar el valor escalado en 'place' para usarlo en el TAC
        return nuevoNodo(place);
    }
    errorSintactico("Factor invalido");
    return NULL;
}

/* ================= TERM ================= */
// Multiplicaciones y divisiones
Nodo* TERM(char *place){
char left[50];
Nodo *izq = FACT(left);

    // Mientras encontremos operadores de multiplicación o división, seguimos analizando términos a la derecha para respetar la jerarquía de operaciones
    while(pos < tablaIndex &&
        strcmp(tabla[pos].token,"OP")==0 &&
        (tabla[pos].lexema[0]=='*' || tabla[pos].lexema[0]=='/')){

            char op = tabla[pos].lexema[0];
            pos++;
        
        /* Inicializar buffer */
        // Es importante inicializar el buffer 'right' antes de llamar a FACT, ya que FACT lo llenará con el valor del factor derecho (ya sea un número, variable o expresión entre paréntesis) para usarlo en el TAC y en la generación del nodo del árbol.
        char right[50] = "";
        Nodo *der = FACT(right);

            if(op == '/'){
                /* detectar si el operando derecho es un numero literal */
                // Si el operando derecho es un número literal (ej: 5 o 3.14), podemos detectar la división entre cero en tiempo de compilación y mostrar un error semántico antes de generar el código intermedio o el árbol sintáctico. Esto es importante para evitar generar código que intente dividir por cero, lo cual causaría un error en tiempo de ejecución.
                if(esEntero(right) || esDecimal(right) || esExponencial(right)){
                    long valor = convertirNumero(right);
                    if(valor == 0){
                        printf("\nError semantico: division entre cero\n");
                        exit(1);
                    }

                }

            }

        char *temp = nuevaTemp();
        generarTAC(temp,left,op,right);
        strcpy(left,temp);
        strcpy(place,left);
        char opStr[2];
            opStr[0] = op;
            opStr[1] = '\0';

            Nodo *padre = nuevoNodo(opStr);
            padre->izq = izq;
            padre->der = der;
            izq = padre;
    }
    strcpy(place,left);
    return izq;
}

/* ================= EXPRESION ================= */
// Sumas y restas
Nodo* EXP(char *place){

    char left[50];
    Nodo *izq = TERM(left);
    strcpy(place,left);
    // Mientras encontremos operadores de suma o resta, seguimos analizando términos a la derecha para respetar la jerarquía de operaciones
    while(pos < tablaIndex &&
        strcmp(tabla[pos].token,"OP")==0 &&
        (tabla[pos].lexema[0]=='+' || tabla[pos].lexema[0]=='-')){

        char op = tabla[pos].lexema[0];
        pos++;
        /*Inicializar buffer */
        // Es importante inicializar el buffer 'right' antes de llamar a TERM, ya que TERM lo llenará con el valor del término derecho (ya sea un número, variable o expresión entre paréntesis) para usarlo en el TAC y en la generación del nodo del árbol.
        char right[50] = "";
        Nodo *der = TERM(right);
        char *temp = nuevaTemp();

        generarTAC(temp,left,op,right);
        strcpy(left,temp);
        strcpy(place,left);

        // Combinar tipos para la expresión resultante: si alguno de los operandos es decimal, el resultado es decimal
        char opStr[2];
        opStr[0]=op;
        opStr[1]='\0';

        Nodo *padre = nuevoNodo(opStr);
        padre->izq = izq;
        padre->der = der;
        izq = padre;

    }

    strcpy(place,left);
    return izq;
}

/* ================= SENTENCIAS ================= */
// Revisa sentencias de asignación como: x = 5 + 3;
void STMTS(){
    while(pos < tablaIndex){
        // Esperar un identificador al inicio de la sentencia
        char nombreVar[50];
        int indiceVar = -1;
        enLadoIzquierdo = 1;   // ← IMPORTANTE

        if(match("ID")){
            strcpy(nombreVar,tabla[pos-1].lexema);
            for(int i=0;i<varIndex;i++){
                if(strcmp(tablaVars[i].nombre,nombreVar)==0){
                    indiceVar = i;
                    break;
                }
            }

            if(indiceVar == -1){
                printf("\nError semantico: variable no declarada -> %s\n",nombreVar);
                exit(1);
            }

        }

        else
            errorSintactico("Se esperaba identificador");

            enLadoIzquierdo = 0;   // ← IMPORTANTE



        // Esperar un operador de asignación '=' después del identificador
        if(!match("ASIG"))
            errorSintactico("Se esperaba =");
        char res[50];
        expresionDecimal = 0;
        Nodo *arbolExp = EXP(res);
        char *tipoVar = obtenerTipo(nombreVar);
        if(strcmp(tipoVar,"ent")==0 && expresionDecimal){
            printf("\nError semantico: no se puede asignar decimal a entero -> %s\n",nombreVar);
            exit(1);
        }

        sprintf(TAC[tacIndex++],"%s = %s",nombreVar,res);
        Nodo *asig = nuevoNodo("=");
        Nodo *varNodo = nuevoNodo(nombreVar);

        asig->izq = varNodo;
        asig->der = arbolExp;
        arboles[arbolIndex++] = asig;

        if(!match("PYC"))
            errorSintactico("Se esperaba ;");

    }

}

/* ================= GENERADOR ASM ================= */
// Verifica si el string es un número literal (para cargarlo directo en AX sin usar memoria)
int esNumeroASM(char *s){
if(s[0]=='-' || s[0]=='+') s++;
    for(int i=0;s[i];i++)
        if(!isdigit(s[i]))
            return 0;
    return 1;
}

// Genera un archivo ASM con el código ensamblador equivalente al programa leído
void generarASM(){
    FILE *f = fopen("programa.asm","w");
    if(!f){
        printf("Error creando ASM\n");
        exit(1);
    }

    // Escribir el encabezado del archivo ASM
    fprintf(f,"org 100h\n\n");
    fprintf(f,"jmp inicio\n\n");

    /* ================= VARIABLES ================= */
    fprintf(f,"; ===== VARIABLES =====\n");

    // Escribe las variables en el archivo ASM
    for(int i=0; i<varIndex; i++)
        fprintf(f,"%s dw 0\n", tablaVars[i].nombre);

    for(int i=1; i<tempIndex; i++)
        fprintf(f,"t%d dw 0\n", i);

    fprintf(f,"\nscale dw %d\n\n", SCALE);

    /* ================= PROCEDIMIENTOS DE IMPRESION ASM ================= */
    fprintf(f,"print_int proc\npush ax\npush bx\npush cx\npush dx\nxor cx,cx\nmov bx,10\ncmp ax,0\njge conv\npush ax\nmov dl,'-'\nmov ah,02h\nint 21h\npop ax\nneg ax\nconv:\nxor dx,dx\ndiv bx\npush dx\ninc cx\ncmp ax,0\njne conv\nloop_print:\npop dx\nadd dl,'0'\nmov ah,02h\nint 21h\nloop loop_print\npop dx\npop cx\npop bx\npop ax\nret\nprint_int endp\n\n");
    fprintf(f,"print_fixed proc\npush ax\npush bx\npush cx\npush dx\ncmp ax,0\njge pf_pos\nneg ax\npush ax\nmov dl,'-'\nmov ah,02h\nint 21h\npop ax\npf_pos:\nmov bx,scale\ncwd\nidiv bx\npush dx\ncall print_int\nmov dl,'.'\nmov ah,02h\nint 21h\npop ax\ncmp ax,0\njge pf_decpos\nneg ax\npf_decpos:\nxor dx,dx\nmov bx,10\ndiv bx\npush dx\nadd al,'0'\nmov dl,al\nmov ah,02h\nint 21h\npop ax\nadd al,'0'\nmov dl,al\nmov ah,02h\nint 21h\nmov dl,13\nmov ah,02h\nint 21h\nmov dl,10\nmov ah,02h\nint 21h\npop dx\npop cx\npop bx\npop ax\nret\nprint_fixed endp\n\n");
    fprintf(f,"inicio:\n\n");
    fprintf(f,"; ===== CODIGO INTERMEDIO =====\n\n");

    // Para cada instrucción en el TAC, generar el código ASM correspondiente
    for(int i=0; i<tacIndex; i++){

        char res[20], op1[20], op2[20];
        char operador;
        fprintf(f,"; %s\n", TAC[i]);

        // Si la instrucción es una operación con dos operandos (ej: t1 = a + b), generar el código ASM para cargar los operandos, realizar la operación y guardar el resultado
        if(sscanf(TAC[i],"%s = %s %c %s", res, op1, &operador, op2) == 4){
            // Cargar Operando 1
            if(esNumeroASM(op1)) fprintf(f,"mov ax,%s\n", op1);
            else fprintf(f,"mov ax,[%s]\n", op1);

            switch(operador){
                case '+':
                    if(esNumeroASM(op2)) fprintf(f,"add ax,%s\n", op2);
                    else fprintf(f,"add ax,[%s]\n", op2);
                    break;
                case '-':
                    if(esNumeroASM(op2)) fprintf(f,"sub ax,%s\n", op2);
                    else fprintf(f,"sub ax,[%s]\n", op2);
                    break;
                case '*':
                    if(esNumeroASM(op2)) fprintf(f,"mov bx,%s\n", op2);
                    else fprintf(f,"mov bx,[%s]\n", op2);
                    fprintf(f,"imul bx\n");      // DX:AX = AX * BX (Está escalado x10000)
                    fprintf(f,"mov bx,scale\n");
                    fprintf(f,"idiv bx\n");      // AX = (DX:AX) / 100 (Vuelve a escala x100)
                    break;
                case '/':
                    // IMPORTANTE: Escalar el dividendo ANTES de dividir
                    fprintf(f,"mov bx,scale\n");
                    fprintf(f,"imul bx\n");      // DX:AX = original_ax * 100
                    if(esNumeroASM(op2)) fprintf(f,"mov bx,%s\n", op2);
                    else fprintf(f,"mov bx,[%s]\n", op2);
                    fprintf(f,"idiv bx\n");      // AX = (AX*100) / op2
                    break;
            }
            fprintf(f,"mov [%s],ax\n\n", res);

        }// Si la instrucción es una asignación simple (ej: x = 5), generar el código ASM para cargar el valor y guardarlo en la variable
        else if(sscanf(TAC[i],"%s = %s", res, op1) == 2){
            if(esNumeroASM(op1)) fprintf(f,"mov ax,%s\n", op1);
            else fprintf(f,"mov ax,[%s]\n", op1);
            fprintf(f,"mov [%s],ax\n\n", res);

        }

    }

    // Después de generar el código para todas las instrucciones intermedias, generar código ASM para imprimir el resultado de las variables declaradas al final del programa
    fprintf(f,"; ===== RESULTADOS =====\n");
    for(int i=0; i<varIndex; i++){
        fprintf(f,"mov ax,[%s]\n", tablaVars[i].nombre);
        fprintf(f,"call print_fixed\n");
    }

    // Finalmente, escribir la instrucción de retorno y cerrar el archivo ASM
    fprintf(f,"\nret\n");
    fclose(f);
    printf("\nASM generado -> programa.asm\n");
}

/* ================= MAIN ================= */

int main(){
    // Leer el código fuente desde un archivo de texto, analizarlo léxicamente para llenar la tabla de símbolos, luego analizarlo sintácticamente para construir los árboles sintácticos y generar el código intermedio (TAC), y finalmente generar el código ASM equivalente.
    char entrada[MAX_ENTRADA] = "";
    char linea[500];
    char nombreArchivo[200];

    printf("Ingrese nombre del archivo (.txt): ");

    // Leer el nombre del archivo desde la entrada estándar
    if(!fgets(nombreArchivo,sizeof(nombreArchivo),stdin))
        return 1;

    // Eliminar el salto de línea al final del nombre del archivo, si existe
    nombreArchivo[strcspn(nombreArchivo,"\n")] = 0;

    // Abrir el archivo para lectura
    FILE *archivo = fopen(nombreArchivo,"r");

    // Si no se pudo abrir el archivo, mostrar un mensaje de error y terminar el programa
    if(!archivo){
        printf("\nNo se pudo abrir el archivo.\n");
        return 1;
    }

    // Leer el contenido del archivo línea por línea y concatenarlo en la variable 'entrada' para su posterior análisis léxico
    while(fgets(linea,sizeof(linea),archivo))
        strncat(entrada,linea,MAX_ENTRADA-strlen(entrada)-1);

    fclose(archivo);

    // Mostrar el contenido leído del archivo para confirmación antes de proceder con el análisis
    printf("\nContenido leido:\n%s\n",entrada);

    // Analizar léxicamente la cadena de entrada para llenar la tabla de símbolos con los tokens encontrados
    analizarCadena(entrada);
    printf("\nIniciando analisis sintactico...\n");
    PROG();

    /* imprimir tabla DESPUES del analisis semantico */
    imprimirTabla();
    printf("\nAnalisis sintactico y semantico correcto.\n");


    printf("\nCODIGO INTERMEDIO (TAC)\n\n");

    for(int i=0;i<tacIndex;i++)
        printf("%s\n",TAC[i]);

    generarASM();
    return 0;

}