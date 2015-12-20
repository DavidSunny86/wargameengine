#pragma once
#include "controller/IScriptHandler.h"
#include <map>

struct lua_State;

class CScriptHandlerLua : public IScriptHandler
{
public:
	CScriptHandlerLua();
	~CScriptHandlerLua();
	virtual void RunScript(std::string const& path) override;
	virtual void CallFunction(std::string const& funcName, FunctionArguments const& arguments = FunctionArguments()) override;
	virtual void RegisterConstant(std::string const& name, std::string const& value) override;
	virtual void RegisterFunction(std::string const& name, FunctionHandler const& handler) override;
	virtual void RegisterMethod(std::string const& className, std::string const& methodName, MethodHandler const& handler) override;
	virtual void RegisterProperty(std::string const& className, std::string const& propertyName, SetterHandler const& setterHandler, GetterHandler const& getterHandler) override;
	virtual void RegisterProperty(std::string const& className, std::string const& propertyName, GetterHandler const& getterHandler) override;
	static void* GetUserData(lua_State *L, int index);
private:
	static int FunctionCallee(lua_State* L);
	static int MethodCallee(lua_State* L);
	static int IndexCallee(lua_State* L);
	static int NewIndexCallee(lua_State* L);
	static int luaError(lua_State *L);
	static int PushReturnValue(lua_State *L, FunctionArgument const& arg);
	static int NewClassInstance(lua_State *L, void* ptr, std::string const& className);
	void RegisterClass(std::string const& className);
	lua_State* m_lua_state;
	std::map<std::string, FunctionHandler> m_functions;
	struct sLuaClass
	{
		std::map<std::string, MethodHandler> methods;
		std::map<std::string, SetterHandler> setters;
		std::map<std::string, GetterHandler> getters;
	};
	static int GetClassAndInstance(lua_State *L, void ** instance, sLuaClass ** classPtr);
	std::map<std::string, sLuaClass> m_classes;
};
