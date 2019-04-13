//#pragma once
//
//#include "Texture.h"
//#include <map>
//
//class MaterialData public MappedValues
//{
//public:
//private:
//};
//
//class Material
//{
//public:
//	Material(const std::string& materialName = "");
//	Material(const Material& other);
//	virtual ~Material();
//
//	Material(const std::string& materialName, const Texture& diffuse, float specularIntensity, float specularPower,
//		const Texture& normalMap = Texture("default_normal.jpg"),
//		const Texture& dispMap = Texture("default_disp.png"), float dispMapScale = 0.0f, float dispMapOffset = 0.0f);
//
//	inline void Setglm::vec3(const std::string& name, const glm::vec3& value) { m_materialData->Setglm::vec3(name, value); }
//	inline void SetFloat(const std::string& name, float value) { m_materialData->SetFloat(name, value); }
//	inline void SetTexture(const std::string& name, const Texture& value) { m_materialData->SetTexture(name, value); }
//
//	inline const glm::vec3& Getglm::vec3(const std::string& name) const { return m_materialData->Getglm::vec3(name); }
//	inline float GetFloat(const std::string& name)              const { return m_materialData->GetFloat(name); }
//	inline const Texture& GetTexture(const std::string& name)   const { return m_materialData->GetTexture(name); }
//protected:
//private:
//	static std::map<std::string, MaterialData*> s_resourceMap;
//	MaterialData* m_materialData;
//	std::string   m_materialName;
//
//	void operator=(const Material& other) {}
//};
