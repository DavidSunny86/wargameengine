#include "..\model\GameModel.h"
#include "..\controller\CommandHandler.h"
#include "LUAScriptHandler.h"
#include "..\model\3dObject.h"
#include "..\model\ObjectGroup.h"

int NewObject(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 5)
        return luaL_error(L, "4 argument expected (model, x, y, rotation)");
	std::string model =  CLUAScript::GetArgument<char*>(2);
	float x = CLUAScript::GetArgument<float>(3);
	float y = CLUAScript::GetArgument<float>(4);
	float rotation = CLUAScript::GetArgument<float>(5);
	IObject* object = new C3DObject(model, x, y, rotation);
	CCommandHandler::GetInstance().lock()->AddNewCreateObject(std::shared_ptr<IObject>(object));
	return CLUAScript::NewInstanceClass(object, "Object");
}

int GetSelectedObject(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = CGameModel::GetIntanse().lock()->GetSelectedObject().get();
	return CLUAScript::NewInstanceClass(object, "Object");
}

int DeleteObject(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	CCommandHandler::GetInstance().lock()->AddNewDeleteObject(std::shared_ptr<IObject>(object));
	object = nullptr;
	return 0;
}

int ObjectNull(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	CLUAScript::SetArgument(object == NULL);
	return 1;
}

int GetObjectModel(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	CLUAScript::SetArgument(object->GetPathToModel().c_str());
	return 1;
}

int GetObjectX(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	CLUAScript::SetArgument(object->GetX());
	return 1;
}

int GetObjectY(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	CLUAScript::SetArgument(object->GetY());
	return 1;
}

int GetObjectZ(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	CLUAScript::SetArgument(object->GetZ());
	return 1;
}

int GetObjectRotation(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	CLUAScript::SetArgument(object->GetRotation());
	return 1;
}

int MoveObject(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 4)
        return luaL_error(L, "3 arguments expected(x, y, z)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	object->Move(CLUAScript::GetArgument<float>(2), CLUAScript::GetArgument<float>(3), CLUAScript::GetArgument<float>(4));
	return 0;
}

int RotateObject(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 2)
        return luaL_error(L, "1 argument expected(rotation)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	object->Rotate(CLUAScript::GetArgument<float>(2));
	return 0;
}

int ShowMesh(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 2)
        return luaL_error(L, "1 argument expected(meshname)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	object->ShowMesh(CLUAScript::GetArgument<char*>(2));
	return 0;
}

int HideMesh(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 2)
        return luaL_error(L, "1 argument expected(meshname)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	object->HideMesh(CLUAScript::GetArgument<char*>(2));
	return 0;
}

int GetProperty(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 2)
		return luaL_error(L, "1 argument expected (key)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	char* key = CLUAScript::GetArgument<char*>(2);
	CLUAScript::SetArgument(object->GetProperty(key).c_str());
	return 1;
}

int SetProperty(lua_State* L)
{
	if(CLUAScript::GetArgumentCount() != 3)
		return luaL_error(L, "2 arguments expected (key, value)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	char* key = CLUAScript::GetArgument<char*>(2);
	char* value = CLUAScript::GetArgument<char*>(3);
	object->SetProperty(key, value);
	return 0;
}

int SetSelectable(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 2)
        return luaL_error(L, "1 argument expected(isSelectable)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	object->SetSelectable(CLUAScript::GetArgument<bool>(2));
	return 0;
}

int SetMoveLimit(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() < 2)
        return luaL_error(L, "at least 1 argument expected(moveLimiterType)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	std::string limiterType = CLUAScript::GetArgument<const char*>(2);
	if(limiterType == "free")
	{
		if (CLUAScript::GetArgumentCount() != 3)//I have no idea why arguments are +1
			return luaL_error(L, "1 argument expected(moveLimiterType)");
		object->SetMovementLimiter(NULL);
	}
	if(limiterType == "static")
	{
		if (CLUAScript::GetArgumentCount() != 3)
			return luaL_error(L, "1 argument expected(moveLimiterType)");
		object->SetMovementLimiter(new CMoveLimiterStatic(object->GetX(), object->GetY(), object->GetZ(), object->GetRotation()));
	}
	if(limiterType == "circle")
	{
		if (CLUAScript::GetArgumentCount() != 6)
			return luaL_error(L, "4 argument expected(moveLimiterType, centerX, centerY, radius)");
		float centerX = CLUAScript::GetArgument<float>(3);
		float centerY = CLUAScript::GetArgument<float>(4);
		float radius = CLUAScript::GetArgument<float>(5);
		object->SetMovementLimiter(new CMoveLimiterCircle(centerX, centerY, radius));
	}
	if(limiterType == "rectangle")
	{
		int n =CLUAScript::GetArgumentCount();
		if (CLUAScript::GetArgumentCount() != 7)
			return luaL_error(L, "5 argument expected(moveLimiterType, x1, y1, x2, y2)");
		float x1 = CLUAScript::GetArgument<float>(3);
		float y1 = CLUAScript::GetArgument<float>(4);
		float x2 = CLUAScript::GetArgument<float>(5);
		float y2 = CLUAScript::GetArgument<float>(6);
		object->SetMovementLimiter(new CMoveLimiterRectangle(x1, y1, x2, y2));
	}
	return 0;
}

int SelectNull(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	std::shared_ptr<IObject> object = NULL;
	CGameModel::GetIntanse().lock()->SelectObject(object);
	return 0;
}

int ObjectEquals(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 2)
        return luaL_error(L, "1 argument expected (secondObject)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	IObject * obj2= (IObject*)CLUAScript::GetArgument<void*>(2);
	CLUAScript::SetArgument(object == obj2);
	return 1;
}

int IsGroup(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	CLUAScript::SetArgument(CGameModel::IsGroup(object));
	return 1;
}

int GetGroupChildrenCount(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 1)
        return luaL_error(L, "no argument expected");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	if(!object) CLUAScript::SetArgument(0);//NULL contains no objects
	if(!CGameModel::IsGroup(object)) CLUAScript::SetArgument(1); //single object
	CObjectGroup * group = (CObjectGroup *)object;
	CLUAScript::SetArgument((int)group->GetCount());
	return 1;
}

int GetGroupChildrenAt(lua_State* L)
{
	if (CLUAScript::GetArgumentCount() != 2)
        return luaL_error(L, "1 argument expected(index)");
	IObject * object = (IObject *)CLUAScript::GetClassInstance("Object");
	size_t index = CLUAScript::GetArgument<int>(2);
	if(!object && !CGameModel::IsGroup(object)) CLUAScript::NewInstanceClass(NULL, "Object");
	CObjectGroup * group = (CObjectGroup *)object;
	if(index > group->GetCount()) CLUAScript::NewInstanceClass(NULL, "Object");
	CLUAScript::NewInstanceClass(group->GetChild(index - 1).get(), "Object");
	return 1;
}

static const luaL_Reg ObjectFuncs[] = {
	{ "New", NewObject },
	{ "GetSelected", GetSelectedObject },
	{ "Delete", DeleteObject },
	{ "Null", ObjectNull },
	{ "GetModel", GetObjectModel },
	{ "GetX", GetObjectX },
	{ "GetY", GetObjectY },
	{ "GetZ", GetObjectZ },
	{ "GetRotation", GetObjectRotation },
	{ "Move", MoveObject },
	{ "Rotate", RotateObject },
	{ "ShowMesh", ShowMesh },
	{ "HideMesh", HideMesh },
	{ "GetProperty", GetProperty },
	{ "SetProperty", SetProperty },
	{ "SetSelectable", SetSelectable },
	{ "SetMoveLimit", SetMoveLimit },
	{ "SelectNull", SelectNull },
	{ "Equals", ObjectEquals },
	{ "IsGroup", IsGroup },
	{ "GetGroupChildrenCount", GetGroupChildrenCount },
	{ "GetGroupChildrenAt", GetGroupChildrenAt },
	{ NULL, NULL }
};

void RegisterObject(CLUAScript & lua)
{
	lua.RegisterClass(ObjectFuncs, "Object");
}