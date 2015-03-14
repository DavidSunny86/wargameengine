#include <map>
#include <vector>
#include "ParticleModel.h"
#include "IParticleEffecth.h"

class CParticleSystem
{
public:
	~CParticleSystem();
	void DrawParticles();
	//World effects
	void AddEffect(std::string const& file, double x, double y, double z, double rotation, double scale, float lifetime);
	//moving effect
	void AddTracer(std::string const& file, CVector3d const& start, CVector3d const& end, double rotation, double scale, double speed);
	//For effects inside models and projectiles
	void DrawEffect(std::string const& effect, float time);
private:
	std::map<std::string, CParticleModel> m_models;
	std::vector<IParticleEffect*> m_effects;
};