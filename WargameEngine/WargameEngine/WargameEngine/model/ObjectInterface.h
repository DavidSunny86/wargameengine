#pragma once
#include <string>
#include <set>
#include <map>
#include "../view/Vector3.h"
#include "MovementLimiter.h"
#include "../view/3dModel.h"

class IObject
{
public:
	virtual std::string GetPathToModel() const = 0;
	virtual ~IObject() {}
	virtual void Move(double x, double y, double z) = 0;
	virtual void GoTo(CVector3d const& coords, double speed, std::string const& animation, float animationSpeed) = 0;
	virtual void SetCoords(double x, double y, double z) = 0;
	virtual void SetCoords(CVector3d const& coords) = 0;
	virtual void Rotate(double rotation) = 0;
	virtual double GetX() const = 0;
	virtual double GetY() const = 0;
	virtual double GetZ() const = 0;
	virtual CVector3d GetCoords() const = 0;
	virtual double GetRotation() const = 0;
	virtual std::set<std::string> const& GetHiddenMeshes() const = 0;
	virtual void HideMesh(std::string const& meshName) = 0;
	virtual void ShowMesh(std::string const& meshName) = 0;
	virtual void SetProperty(std::string const& key, std::string const& value) = 0;
	virtual std::string const GetProperty(std::string const& key) const = 0;
	virtual bool IsSelectable() const = 0;
	virtual void SetSelectable(bool selectable) = 0;
	virtual void SetMovementLimiter(IMoveLimiter * limiter) = 0;
	virtual std::map<std::string, std::string> const& GetAllProperties() const = 0;
	virtual bool CastsShadow() const = 0;
	virtual void PlayAnimation(std::string const& animation, sAnimation::eLoopMode loop = sAnimation::NONLOOPING, float speed = 1.0f) = 0;
	virtual std::string GetAnimation() const = 0;
	virtual float GetAnimationTime() const = 0;
	virtual void AddSecondaryModel(std::string const& model) = 0;
	virtual void RemoveSecondaryModel(std::string const& model) = 0;
	virtual unsigned int GetSecondaryModelsCount() const = 0;
	virtual std::string GetSecondaryModel(unsigned int index) const = 0;
	virtual sAnimation::eLoopMode GetAnimationLoop() const = 0;
	virtual float GetAnimationSpeed() const = 0;
	virtual void Update() = 0;
	virtual std::map<std::string, unsigned char[3]> const& GetTeamColor() const = 0;
	virtual void ApplyTeamColor(std::string const& suffix, unsigned char r, unsigned char g, unsigned char b) = 0;
};