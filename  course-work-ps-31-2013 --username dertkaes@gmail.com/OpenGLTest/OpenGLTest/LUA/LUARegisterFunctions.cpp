#include "LUAScriptHandler.h"
#include "..\view\GameView.h"
#include "..\controller\CommandHandler.h"
#include "..\view\Input.h"
#include <Windows.h>
#include "..\los.h"

int CreateTable(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 3)
        return luaL_error(L, "3 argument expected (width, height, texture)");
	float width = CLUAScript::GetArgument<float>(1);
	float height = CLUAScript::GetArgument<float>(2);
	std::string texture = CLUAScript::GetArgument<char*>(3);
	CGameView::GetIntanse().lock()->CreateTable(width, height, texture);
	return 0;
}

int CreateSkybox(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 2)
        return luaL_error(L, "2 argument expected (size, texture folder)");
	float size = CLUAScript::GetArgument<float>(1);
	std::string texture = CLUAScript::GetArgument<char*>(2);
	CGameView::GetIntanse().lock()->CreateSkybox(size, texture);
	return 0;
}

int CameraSetLimits(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 4)
        return luaL_error(L, "4 argument expected (max trans x, max trans y, max scale, min scale)");
	float maxTransX = CLUAScript::GetArgument<float>(1);
	float maxTransY = CLUAScript::GetArgument<float>(2);
	float maxScale = CLUAScript::GetArgument<float>(3);
	float minScale = CLUAScript::GetArgument<float>(4);
	CGameView::GetIntanse().lock()->CameraSetLimits(maxTransX, maxTransY, maxScale, minScale);
	return 0;
}

int LoS(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 2)
        return luaL_error(L, "2 argument expected (max trans x, max trans y, max scale, min scale)");
	IObject* shootingModel = (IObject*)CLUAScript::GetArgument<void*>(1);
	IObject* target = (IObject*)CLUAScript::GetArgument<void*>(2);
	int los = Los(shootingModel, target);
	CLUAScript::SetArgument(los);
	return 1;
}

int Ruler(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 0)
        return luaL_error(L, "no arguments expected");
	CInput::EnableRuler();
	return 0;
}

int Undo(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 0)
        return luaL_error(L, "no arguments expected");
	CCommandHandler::GetInstance().lock()->Undo();
	return 0;
}

int Redo(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 0)
        return luaL_error(L, "no arguments expected");
	CCommandHandler::GetInstance().lock()->Redo();
	return 0;
}

int ShowMessageBox(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() < 1 || CLUAScript::GetArgumentCount() > 2)
        return luaL_error(L, "1 or 2 argument expected (caption, text)");
	char* text =  CLUAScript::GetArgument<char*>(1);
	char* caption = "";
	if(CLUAScript::GetArgumentCount() == 2)
		caption = CLUAScript::GetArgument<char*>(2);
	MessageBoxA(NULL, text, caption,0);
	return 0;
}

int RunScript(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 1)
		return luaL_error(L, "1 argument expected (filename)");
	char* filename =  CLUAScript::GetArgument<char*>(1);
	CLUAScript::RunScript(filename);
	return 0;
}

int GetGlobalProperty(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 1)
		return luaL_error(L, "1 argument expected (key)");
	char* key = CLUAScript::GetArgument<char*>(1);
	CLUAScript::SetArgument(CGameModel::GetIntanse().lock()->GetProperty(key).c_str());
	return 1;
}

int SetGlobalProperty(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 2)
		return luaL_error(L, "2 arguments expected (key, value)");
	char* key = CLUAScript::GetArgument<char*>(1);
	char* value = CLUAScript::GetArgument<char*>(2);
	CGameModel::GetIntanse().lock()->SetProperty(key, value);
	return 0;
}

int IncludeLibrary(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 1)
		return luaL_error(L, "1 argument expected (libName)");
	char* lib = CLUAScript::GetArgument<char*>(1);
	CLUAScript::IncludeLibrary(lib);
	return 0;
}

int SetSelectionCallback(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 1)
		return luaL_error(L, "1 argument expected (funcName)");
	std::string func = CLUAScript::GetArgument<char*>(1);
	if(func.empty())
	{
		CGameView::GetIntanse().lock()->SetSelectionCallback(std::function<void()>());
	}
	else
	{
		auto function = [func]()
		{ 
			CLUAScript::CallFunction(func);
		};
		CGameView::GetIntanse().lock()->SetSelectionCallback(function);
	}
	return 0;
}

int SetUpdateCallback(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 1)
		return luaL_error(L, "1 argument expected (funcName)");
	std::string func = CLUAScript::GetArgument<char*>(1);
	if(func.empty())
	{
		CGameView::GetIntanse().lock()->SetUpdateCallback(std::function<void()>());
	}
	else
	{
		auto function = [func]()
		{ 
			CLUAScript::CallFunction(func);
		};
		CGameView::GetIntanse().lock()->SetUpdateCallback(function);
	}
	return 0;
}

int SetSingleCallback(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 1)
		return luaL_error(L, "1 argument expected (funcName)");
	std::string func = CLUAScript::GetArgument<char*>(1);
	if(!func.empty())
	{
		auto function = [func]()
		{ 
			CLUAScript::CallFunction(func);
		};
		CGameView::GetIntanse().lock()->SetSingleCallback(function);
	}
	return 0;
}

int ResizeWindow(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 2)
		return luaL_error(L, "2 argument expected (height, width)");
	int height = CLUAScript::GetArgument<int>(1);
	int width = CLUAScript::GetArgument<int>(2);
	CGameView::GetIntanse().lock()->ResizeWindow(height, width);
	return 0;
}

void RegisterFunctions(CLUAScript & lua)
{
	lua.RegisterConstant(CreateTable, "CreateTable");
	lua.RegisterConstant(CreateSkybox, "CreateSkybox");
	lua.RegisterConstant(CameraSetLimits, "CameraSetLimits");
	lua.RegisterConstant(Ruler, "Ruler");
	lua.RegisterConstant(Undo, "Undo");
	lua.RegisterConstant(Redo, "Redo");
	lua.RegisterConstant(ShowMessageBox, "MessageBox");
	lua.RegisterConstant(RunScript, "RunScript");
	lua.RegisterConstant(GetGlobalProperty, "GetGlobalProperty");
	lua.RegisterConstant(SetGlobalProperty, "SetGlobalProperty");
	lua.RegisterConstant(IncludeLibrary, "IncludeLibrary");
	lua.RegisterConstant(SetSelectionCallback, "SetSelectionCallback");
	lua.RegisterConstant(SetUpdateCallback, "SetUpdateCallback");
	lua.RegisterConstant(SetSingleCallback, "SetSingleCallback");
	lua.RegisterConstant(LoS, "LoS");
	lua.RegisterConstant(ResizeWindow, "ResizeWindow");
}