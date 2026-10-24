#include "tokenizer.h"
#include <cctype>

std::vector<Token> tokenize(const std::string &input)
{
    std::vector<Token> tokens;
    size_t i = 0, n = input.size();
    bool expectCommand = true;
    bool afterRedirect = false;

    while (i < n)
    {
        if (isspace((unsigned char)input[i]))
        {
            i++;
            continue;
        }

        Token token;
        char c = input[i];

        if (c == '|')
        {
            token.type = TOKEN_PIPE;
            token.value = "|";
            i++;
            expectCommand = true;
            afterRedirect = false;
        }
        else if (c == '<')
        {
            token.type = TOKEN_INPUT;
            token.value = "<";
            i++;
            afterRedirect = true;
        }
        else if (c == '&')
        {
            token.type = TOKEN_BACKGROUND;
            token.value = "&";
            i++;
            afterRedirect = false;
        }
        else if (c == '>')
        {
            if (input.compare(i, 3, ">>&") == 0)
            {
                token.type = TOKEN_REDIRECT_AND_ERROR;
                token.value = ">>&";
                i += 3;
            }
            else if (input.compare(i, 2, ">>") == 0)
            {
                token.type = TOKEN_APPEND;
                token.value = ">>";
                i += 2;
            }
            else
            {
                token.type = TOKEN_REDIRECT;
                token.value = ">";
                i++;
            }
            afterRedirect = true;
        }
        else if (c == '2' && i + 1 < n && input[i + 1] == '>')
        {
            token.type = TOKEN_ERROR;
            token.value = "2>";
            i += 2;
            afterRedirect = true;
        }
        else
        {
            std::string word;
            while (i < n && !isspace((unsigned char)input[i]) &&
                   input[i] != '|' && input[i] != '<' &&
                   input[i] != '>' && input[i] != '&')
            {
                if (input[i] == '"' || input[i] == '\'')
                {
                    char q = input[i++];
                    while (i < n && input[i] != q)
                        word += input[i++];
                    if (i < n)
                        i++;
                }
                else
                {
                    word += input[i++];
                }
            }

            if (afterRedirect)
            {
                token.type = TOKEN_ARGUMENT;
                afterRedirect = false;
            }
            else if (expectCommand)
            {
                token.type = TOKEN_COMMAND;
                expectCommand = false;
            }
            else
            {
                token.type = TOKEN_ARGUMENT;
            }
            token.value = word;
        }
        tokens.push_back(token);
    }

    tokens.push_back({TOKEN_EOF, ""});
    return tokens;
}
