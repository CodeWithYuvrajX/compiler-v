#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 256
#define MAX_TEXT 128

typedef struct {
    char text[MAX_TEXT];
    int type;
} Token;

enum { TOKEN_OPERAND, TOKEN_OPERATOR, TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_END };

static Token tokens[MAX_TOKENS];
static int token_count;
static int position;
static int temp_count;
static char temp_names[MAX_TOKENS][32];
static char error_message[160];

int isOperator(char character) {
    return character == '+' || character == '-' || character == '*' || character == '/' || character == '%';
}

int precedence(char operator) {
    if (operator == '*' || operator == '/' || operator == '%') return 2;
    if (operator == '+' || operator == '-') return 1;
    return 0;
}

void setError(const char *message) {
    if (error_message[0] == '\0') snprintf(error_message, sizeof(error_message), "%s", message);
}

int addToken(const char *text, int type) {
    if (token_count >= MAX_TOKENS - 1) {
        setError("Expression is too long.");
        return 0;
    }
    snprintf(tokens[token_count].text, MAX_TEXT, "%s", text);
    tokens[token_count].type = type;
    token_count++;
    return 1;
}

int tokenize(const char *expression) {
    int index = 0;
    token_count = 0;
    while (expression[index] != '\0') {
        char buffer[MAX_TEXT];
        int length = 0;
        if (isspace((unsigned char) expression[index])) {
            index++;
            continue;
        }
        if (isalpha((unsigned char) expression[index]) || expression[index] == '_') {
            while (isalnum((unsigned char) expression[index]) || expression[index] == '_') {
                if (length < MAX_TEXT - 1) buffer[length++] = expression[index];
                index++;
            }
            buffer[length] = '\0';
            if (!addToken(buffer, TOKEN_OPERAND)) return 0;
            continue;
        }
        if (isdigit((unsigned char) expression[index]) || expression[index] == '.') {
            int dots = 0;
            while (isdigit((unsigned char) expression[index]) || expression[index] == '.') {
                if (expression[index] == '.') dots++;
                if (length < MAX_TEXT - 1) buffer[length++] = expression[index];
                index++;
            }
            buffer[length] = '\0';
            if (dots > 1) {
                setError("A number cannot contain more than one decimal point.");
                return 0;
            }
            if (!addToken(buffer, TOKEN_OPERAND)) return 0;
            continue;
        }
        if (isOperator(expression[index])) {
            buffer[0] = expression[index++];
            buffer[1] = '\0';
            if (!addToken(buffer, TOKEN_OPERATOR)) return 0;
            continue;
        }
        if (expression[index] == '(') {
            index++;
            if (!addToken("(", TOKEN_LPAREN)) return 0;
            continue;
        }
        if (expression[index] == ')') {
            index++;
            if (!addToken(")", TOKEN_RPAREN)) return 0;
            continue;
        }
        setError("Unsupported character in expression.");
        return 0;
    }
    tokens[token_count].type = TOKEN_END;
    return token_count > 0;
}

const char *generateTemp(void) {
    int index = temp_count++;
    snprintf(temp_names[index], sizeof(temp_names[index]), "t%d", temp_count);
    return temp_names[index];
}

const char *parseExpression(FILE *output);

const char *parsePrimary(FILE *output) {
    Token current = tokens[position];
    if (current.type == TOKEN_OPERAND) {
        position++;
        return tokens[position - 1].text;
    }
    if (current.type == TOKEN_LPAREN) {
        const char *value;
        position++;
        value = parseExpression(output);
        if (tokens[position].type != TOKEN_RPAREN) {
            setError("Missing closing parenthesis.");
            return "";
        }
        position++;
        return value;
    }
    setError("Expected a variable, number, or opening parenthesis.");
    return "";
}

const char *parseTerm(FILE *output) {
    const char *left = parsePrimary(output);
    while (tokens[position].type == TOKEN_OPERATOR && precedence(tokens[position].text[0]) == 2) {
        char operator = tokens[position].text[0];
        const char *right;
        const char *temp;
        position++;
        right = parsePrimary(output);
        if (error_message[0] != '\0') return "";
        temp = generateTemp();
        fprintf(output, "%s = %s %c %s\n", temp, left, operator, right);
        left = temp;
    }
    return left;
}

const char *parseExpression(FILE *output) {
    const char *left = parseTerm(output);
    while (tokens[position].type == TOKEN_OPERATOR && precedence(tokens[position].text[0]) == 1) {
        char operator = tokens[position].text[0];
        const char *right;
        const char *temp;
        position++;
        right = parseTerm(output);
        if (error_message[0] != '\0') return "";
        temp = generateTemp();
        fprintf(output, "%s = %s %c %s\n", temp, left, operator, right);
        left = temp;
    }
    return left;
}

int generateTAC(const char *expression) {
    char destination[MAX_TEXT];
    char *equals;
    FILE *output;
    int destination_length;

    snprintf(destination, sizeof(destination), "%s", expression);
    equals = strchr(destination, '=');
    if (equals == NULL) {
        setError("Use an assignment such as result = a + b.");
        return 0;
    }
    *equals = '\0';
    destination_length = (int) strlen(destination) - 1;
    while (destination_length >= 0 && isspace((unsigned char) destination[destination_length])) destination[destination_length--] = '\0';
    if (destination[0] == '\0') {
        setError("The assignment needs a destination variable.");
        return 0;
    }
    for (int index = 0; destination[index] != '\0'; index++) {
        if (!(isalnum((unsigned char) destination[index]) || destination[index] == '_')) {
            setError("The destination must be a variable name.");
            return 0;
        }
    }
    if (!tokenize(equals + 1)) {
        if (error_message[0] == '\0') setError("Expression cannot be empty.");
        return 0;
    }
    position = 0;
    temp_count = 0;
    output = fopen("tac_output.tmp", "w");
    if (output == NULL) {
        setError("Could not create the TAC output.");
        return 0;
    }
    {
        const char *result = parseExpression(output);
        if (error_message[0] == '\0' && tokens[position].type != TOKEN_END) setError("Unexpected token after the expression.");
        if (error_message[0] == '\0') fprintf(output, "%s = %s\n", destination, result);
    }
    fclose(output);
    if (error_message[0] != '\0') {
        remove("tac_output.tmp");
        return 0;
    }
    output = fopen("tac_output.tmp", "r");
    if (output == NULL) {
        setError("Could not read the generated TAC.");
        return 0;
    }
    {
        char line[256];
        while (fgets(line, sizeof(line), output) != NULL) fputs(line, stdout);
    }
    fclose(output);
    remove("tac_output.tmp");
    return 1;
}

int main(void) {
    char expression[1024];
    size_t length;
    if (fgets(expression, sizeof(expression), stdin) == NULL) {
        fprintf(stderr, "Invalid Expression\nPlease enter an expression.");
        return 1;
    }
    length = strlen(expression);
    while (length > 0 && (expression[length - 1] == '\n' || expression[length - 1] == '\r')) expression[--length] = '\0';
    error_message[0] = '\0';
    if (!generateTAC(expression)) {
        fprintf(stderr, "Invalid Expression\n%s", error_message[0] == '\0' ? "Please check the operator or parentheses." : error_message);
        return 1;
    }
    return 0;
}