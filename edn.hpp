#include <list>
#include <string>
#include <iostream>
namespace edn { 
  using std::cout;
  using std::endl;
  using std::string;
  using std::list;

  enum TokenType {
    TokenString,
    TokenAtom,
    TokenParen
  };

  struct EdnToken {
    TokenType type;
    int line;
    string value;
  };

  enum NodeType {
    EdnNil,
    EdnSymbol,
    EdnKeyword,
    EdnBool,
    EdnInt,
    EdnFloat,
    EdnString, 
    EdnChar, 

    EdnList,
    EdnVector,
    EdnMap,
    EdnSet,

    EdnDiscard,
    EdnTagged
  };

  struct EdnNode {
    NodeType type;
    int line;
    string value;
    list<EdnNode> values;
  };

  void createToken(TokenType type, int line, string value, list<EdnToken> &tokens) {
    EdnToken token;
    token.type = type;
    token.line = line;
    token.value = value;
    tokens.push_back(token);
  }


  list<EdnToken> lex(string edn) {
    string::iterator it;
    int line = 1;
    char escapeChar = '\\';
    bool escaping = false;
    bool inString = false;
    string stringContent = "";
    bool inComment = false;
    string token = "";
    string paren = "";
    list<EdnToken> tokens;

    for (it = edn.begin(); it != edn.end(); ++it) {
      if (*it == '\n' || *it == '\r') line++;

      if (!inString && *it == ';' && !escaping) inComment = true;

      if (inComment) {
        if (*it == '\n') {
          inComment = false;
          if (token != "") {
              createToken(TokenAtom, line, token, tokens);
              token = "";
          }
          continue;
        }
      }
    
      if (*it == '"' && !escaping) {
        if (inString) {
          createToken(TokenString, line, stringContent, tokens);
          inString = false;
        } else {
          stringContent = "";
          inString = true;
        }
        continue;
      }

      if (inString) { 
        if (*it == escapeChar && !escaping) {
          escaping = true;
          continue;
        }

        if (escaping) {
          escaping = false;
          if (*it == 't' || *it == 'n' || *it == 'f' || *it == 'r') stringContent += escapeChar;
        }
        stringContent += *it;   
      } else if (*it == '(' || *it == ')' || *it == '[' || *it == ']' || *it == '{' || 
                 *it == '}' || *it == '\t' || *it == '\n' || *it == '\r' || * it == ' ') {
        if (token != "") { 
          createToken(TokenAtom, line, token, tokens);
          token = "";
        } else {
          if (*it == '(' || *it == ')' || *it == '[' || *it == ']' || *it == '{' || *it == '}') {
            paren = "";
            paren += *it;
            createToken(TokenParen, line, paren, tokens);
          }
        } 
      } else {
        if (escaping) { 
          escaping = false;
        } else if (*it == escapeChar) {
          escaping = true;
        }

        if (token == "#_") {
          createToken(TokenAtom, line, token, tokens); 
          token = "";
        }
        token += *it;
      }
    }
    if (token != "") {
      createToken(TokenAtom, line, token, tokens); 
    }

    return tokens;
  }

  bool validSymbol(string value) {
    return false;
  }

  bool validNil(string value) {
    return (value == "nil");
  }

  bool validBool(string value) {
    return (value == "true" || value == "false");
  }

  bool validInt(string value) {
    return false;
  }

  bool validFloat(string value) {
    return false;
  }

  bool validChar(string value) {
    return (value.at(0) == '\\' && value.length() == 2);
  }

  EdnNode handleAtom(EdnToken token) {
    EdnNode node;
    node.type = EdnString; // just for debug until rest of valid funcs are working
    node.line = token.line;
    node.value = token.value;
    cout << "EDN TOKEN " << token.value << endl;

    if (validNil(token.value)) { 
      node.type = EdnNil;
    } 
    else if (validSymbol(token.value)) { 
      if (token.value.at(0) == ':') {
        node.type = EdnKeyword;
      } else {
        node.type = EdnSymbol; 
      }
    }
    else if (token.type == TokenString) { 
      node.type = EdnString;
    }
    else if (validChar(token.value)) {
      node.type = EdnChar;
    }
    else if (validBool(token.value)) { 
      node.type = EdnBool;
    }
    else if (validInt(token.value)) {
      node.type = EdnInt;
    }
    else if (validFloat(token.value)) {
      node.type = EdnFloat;
    }

    return node;
  }

  EdnNode handleCollection(EdnToken token, list<EdnNode> values) {
    EdnNode node;
    node.line = token.line;
    node.values = values;

    if (token.value == "(") {
      cout << "GOT A LIST" << endl;
      node.type = EdnList;
    }
    else if (token.value == "[") {
      cout << "GOT A VECTOR" << endl;
      node.type = EdnVector;
    }
    if (token.value == "{") {
      cout << "GOT A MAP" << endl;
      node.type = EdnMap;
    }
    return node;
  }

  EdnNode handleTagged(EdnToken token, EdnNode value) { 
    EdnNode node;
    node.line = token.line;

    string tagName = token.value.substr(1, token.value.length() - 2);

    if (tagName == "_") {
      cout << "DISCARD" << endl;
      node.type = EdnDiscard;
    } else if (tagName == "") {
      cout << "SET" << endl;
      //special case where we return early as # { is a set - thus tagname is empty
      node.type = EdnSet;
      if (value.type != EdnMap) {
        throw "Was expection a { } after hash to build set";
      }
      node.values = value.values;
      return node;
    } else {
      cout << "EDN TAGGED: " << tagName << endl;
      node.type = EdnTagged;
    }
  
    if (!validSymbol(tagName)) {
      throw "Invalid tag name at line: "; 
    }

    EdnToken symToken;
    symToken.type = TokenAtom;
    symToken.line = token.line;
    symToken.value = tagName;
  
    list<EdnNode> values;
    values.push_back(handleAtom(symToken)); 
    values.push_back(value);

    node.values = values;
    return node; 
  }

  EdnToken shiftToken(list<EdnToken> &tokens) { 
    EdnToken nextToken = tokens.front();
    tokens.pop_front();
    return nextToken;
  }

  EdnNode readAhead(EdnToken token, list<EdnToken> &tokens) {
    if (token.type == TokenParen) {

      EdnToken nextToken;
      list<EdnNode> L;
      string closeParen;
      if (token.value == "(") closeParen = ")";
      if (token.value == "[") closeParen = "]"; 
      if (token.value == "{") closeParen = "}"; 

      cout << "GOT PAREN " << token.value << " MATCH WITH " << closeParen << endl;

      while (true) {
        if (tokens.empty()) throw "unexpected end of list";

        nextToken = shiftToken(tokens);

        if (nextToken.value == closeParen) {
          return handleCollection(token, L);
        } else {
          L.push_back(readAhead(nextToken, tokens));
        }
      }
    } else if (token.value == ")" || token.value == "]" || token.value == "}") {
      throw "Unexpected " + token.value;
    } else {
      if (token.value.at(0) == '#') {
        return handleTagged(token, readAhead(shiftToken(tokens), tokens));
      } else {
        return handleAtom(token);
      }
    }
  }

  EdnNode read(string edn) {
    list<EdnToken> tokens = lex(edn);

    if (tokens.size() == 0) {
      throw "No parsable tokens found in string";
    }

    return readAhead(shiftToken(tokens), tokens);
  }
}