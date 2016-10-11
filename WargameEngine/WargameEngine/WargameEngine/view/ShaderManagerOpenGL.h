#pragma once
#include <string>
#include "IShaderManager.h"

class CShaderManagerOpenGL : public IShaderManager
{
public:
	CShaderManagerOpenGL();
	void NewProgram(std::wstring const& vertex = L"", std::wstring const& fragment = L"", std::wstring const& geometry = L"") override;
	void BindProgram() const override;
	void UnBindProgram() const override;

	virtual void SetUniformValue(std::string const& uniform, int count, const float* value) const override;
	virtual void SetUniformValue(std::string const& uniform, int count, const int* value) const override;
	virtual void SetUniformValue(std::string const& uniform, int count, const unsigned int* value) const override;
	virtual void SetUniformValue2(std::string const& uniform, int count, const float* value) const override;
	virtual void SetUniformValue2(std::string const& uniform, int count, const int* value) const override;
	virtual void SetUniformValue2(std::string const& uniform, int count, const unsigned int* value) const override;
	virtual void SetUniformValue3(std::string const& uniform, int count, const float* value) const override;
	virtual void SetUniformValue3(std::string const& uniform, int count, const int* value) const override;
	virtual void SetUniformValue3(std::string const& uniform, int count, const unsigned int* value) const override;
	virtual void SetUniformValue4(std::string const& uniform, int count, const float* value) const override;
	virtual void SetUniformValue4(std::string const& uniform, int count, const int* value) const override;
	virtual void SetUniformValue4(std::string const& uniform, int count, const unsigned int* value) const override;
	virtual void SetUniformMatrix4(std::string const& uniform, size_t count, float* value) const override;

	virtual void SetVertexAttribute(std::string const& attribute, int elementSize, size_t totalSize, float* values) const override;
	virtual void SetVertexAttribute(std::string const& attribute, int elementSize, size_t totalSize, int* values) const override;
	virtual void SetVertexAttribute(std::string const& attribute, int elementSize, size_t totalSize, unsigned int* values) const override;

	virtual void SetPerInstanceVertexAttribute(std::string const& attribute, int elementSize, size_t totalSize, float* values) const override;

	virtual void DisableVertexAttribute(std::string const& attribute, int size, float* defaultValue) const override;
	virtual void DisableVertexAttribute(std::string const& attribute, int size, int* defaultValue) const override;
	virtual void DisableVertexAttribute(std::string const& attribute, int size, unsigned int* defaultValue) const override;
private:
	unsigned int m_program;
};