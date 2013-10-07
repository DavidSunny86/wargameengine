#include <string>

class CSkyBox
{
public:
	CSkyBox(double x, double y, double z, double width, double height, double length, std::string const& imageFolder)
		:m_width(width), m_height(height), m_length(length), m_x(x - width  / 2), m_y(y - height / 2), m_z(z - length / 2),
		m_imageFolder(imageFolder) {}
	void Draw();
private:
	double m_height;
	double m_width;
	double m_length;
	double m_x;
	double m_y;
	double m_z;
	std::string m_imageFolder;
};