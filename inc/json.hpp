#ifndef OBC_JSON_HPP
#define OBC_JSON_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include <variant>
#include <memory>

class JsonList;
class JsonObject;

using JsonType = std::variant<long long, 
                              double, 
                              std::string, 
                              JsonList, 
                              JsonObject>;

class JsonList {
public:
    JsonList();

private:
    std::vector<JsonType> m_list;
};

class JsonObject {
public:
    JsonObject();

private:
    std::unordered_map<std::string, JsonType> m_map;
};

class JsonToken {
public:
    enum Type {

    };

    JsonToken(std::string lexeme, Type type);

private:
    std::string m_lexeme;
    Type m_type;
};

class JsonLexer {
public:
    JsonLexer(std::string text);

private:
    std::string m_text;
};

class JsonParser {
public:

private:

};

#endif
