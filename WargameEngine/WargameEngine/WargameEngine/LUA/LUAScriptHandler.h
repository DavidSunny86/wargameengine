#ifdef _WINDOWS
#pragma comment(lib,"lua.lib")
#endif
#include <lua.hpp>
#include <string>
#include <vector>
#pragma once

class CLUAScript
{
public:
	CLUAScript();
	~CLUAScript();
	static int RunScript(std::string const& file);
	static int GetArgumentCount();
	template<class T>
	static T GetArgument(int index);
	template<class T>
	static void SetArgument(T arg);
	template<class T>
    static void RegisterConstant(T value, char* constantname);
	static void RegisterClass(const luaL_Reg funcs[], std::string const& className);
	static void CallFunction(std::string const& funcName);
	template<class T>
	static void CallFunction(std::string const& funcName, T param);
	static void * GetClassInstance(std::string const& className);
	static int NewInstanceClass(void* instance, std::string const& className);
	static void IncludeLibrary(std::string const& libName);
	template<class T>
	static std::vector<T> GetArray(int index);
private:
	static lua_State* m_lua_state;
};