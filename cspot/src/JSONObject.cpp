#include "JSONObject.h"
#include <stdlib.h>

JSONValue::JSONValue(cJSON *body, std::string key)
{
    this->body = body;
    this->key = key;
}

void JSONValue::operator=(const std::string val)
{
    this->operator=(val.c_str());
}
void JSONValue::operator=(const char *val)
{
    cJSON_AddStringToObject(this->body, this->key.c_str(), val);
}
void JSONValue::operator=(int val)
{
    cJSON_AddNumberToObject(this->body, this->key.c_str(), val);
}

JSONObject::JSONObject()
{
    this->body = cJSON_CreateObject();
}

JSONObject::~JSONObject()
{
    cJSON_Delete(this->body);
}

JSONValue JSONObject::operator[](std::string index)
{
    return JSONValue(this->body, index);
}

std::string JSONObject::toString()
{
    char *body = cJSON_Print(this->body);
    std::string retVal = std::string(body);
    free(body);
    return retVal;
    
}