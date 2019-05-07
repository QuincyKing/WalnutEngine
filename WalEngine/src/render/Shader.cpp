#include "RenderEngine.h"
#include "Shader.h"
#include "../util/Util.h"
#include "Material.h"
#include <cassert>

///////////////////////////////////////
////////////ShaderData/////////////////
///////////////////////////////////////

int ShaderData::SupportedOpenGLLevel = 0;
std::string ShaderData::GlslVersion = "";
std::map<std::string, ShaderData*> Shader::ResourceMap;

static std::string load_shader(const std::string& fileName)
{
	std::ifstream file;
	file.open(("./shader/" + fileName).c_str());

	std::string output;
	std::string line;

	if (file.is_open())
	{
		while (file.good())
		{
			getline(file, line);

			if (line.find("#include") == std::string::npos)
				output.append(line + "\n");
			else
			{
				std::string includeFileName = Util::split(line, ' ')[1];
				includeFileName = includeFileName.substr(1, includeFileName.length() - 2);

				std::string toAppend = load_shader(includeFileName);
				output.append(toAppend + "\n");
			}
		}
	}
	else
	{
		std::cerr << "Unable to load shader: " << fileName << std::endl;
	}

	return output;
};

static std::vector<TypedData> find_uniform_struct_components(const std::string& openingBraceToClosingBrace)
{
	static const char charsToIgnore[] = { ' ', '\n', '\t', '{' };
	static const size_t UNSIGNED_NEG_ONE = (size_t)-1;

	std::vector<TypedData> result;
	std::vector<std::string> structLines = Util::split(openingBraceToClosingBrace, ';');

	for (unsigned int i = 0; i < structLines.size(); i++)
	{
		size_t nameBegin = UNSIGNED_NEG_ONE;
		size_t nameEnd = UNSIGNED_NEG_ONE;

		for (unsigned int j = 0; j < structLines[i].length(); j++)
		{
			bool isIgnoreableCharacter = false;

			for (unsigned int k = 0; k < sizeof(charsToIgnore) / sizeof(char); k++)
			{
				if (structLines[i][j] == charsToIgnore[k])
				{
					isIgnoreableCharacter = true;
					break;
				}
			}

			if (nameBegin == UNSIGNED_NEG_ONE && isIgnoreableCharacter == false)
			{
				nameBegin = j;
			}
			else if (nameBegin != UNSIGNED_NEG_ONE && isIgnoreableCharacter)
			{
				nameEnd = j;
				break;
			}
		}

		if (nameBegin == UNSIGNED_NEG_ONE || nameEnd == UNSIGNED_NEG_ONE)
			continue;

		TypedData newData(
			structLines[i].substr(nameEnd + 1),
			structLines[i].substr(nameBegin, nameEnd - nameBegin));

		result.push_back(newData);
	}

	return result;
}

static std::string find_uniform_struct_name(const std::string& structStartToOpeningBrace)
{
	return Util::split(Util::split(structStartToOpeningBrace, ' ')[0], '\n')[0];
}

static std::vector<UniformStruct> find_uniform_structs(const std::string& shaderText)
{
	static const std::string STRUCT_KEY = "struct";
	std::vector<UniformStruct> result;

	size_t structLocation = shaderText.find(STRUCT_KEY);
	while (structLocation != std::string::npos)
	{
		structLocation += STRUCT_KEY.length() + 1; //Ignore the struct keyword and space

		size_t braceOpening = shaderText.find("{", structLocation);
		size_t braceClosing = shaderText.find("}", braceOpening);

		UniformStruct newStruct(
			find_uniform_struct_name(shaderText.substr(structLocation, braceOpening - structLocation)),
			find_uniform_struct_components(shaderText.substr(braceOpening, braceClosing - braceOpening)));

		result.push_back(newStruct);
		structLocation = shaderText.find(STRUCT_KEY, structLocation);
	}

	return result;
}

static void check_shader_error(int shader, int flag, bool isProgram, const std::string& errorMessage)
{
	GLint success = 0;
	GLchar error[1024] = { 0 };

	if (isProgram)
		glGetProgramiv(shader, flag, &success);
	else
		glGetShaderiv(shader, flag, &success);

	if (!success)
	{
		if (isProgram)
			glGetProgramInfoLog(shader, sizeof(error), NULL, error);
		else
			glGetShaderInfoLog(shader, sizeof(error), NULL, error);

		fprintf(stderr, "%s: '%s'\n", errorMessage.c_str(), error);
	}
}

ShaderData::ShaderData()
{
}

void ShaderData::init(const std::string& file, int type)
{
	if(mProgram == 0)
		mProgram = glCreateProgram();

	if (mProgram == 0)
	{
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	if (SupportedOpenGLLevel == 0)
	{
		int majorVersion;
		int minorVersion;

		glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
		glGetIntegerv(GL_MINOR_VERSION, &minorVersion);

		SupportedOpenGLLevel = majorVersion * 100 + minorVersion * 10;

		if (SupportedOpenGLLevel >= 330)
		{
			std::ostringstream os;
			os << SupportedOpenGLLevel;

			GlslVersion = os.str();
		}
		else if (SupportedOpenGLLevel >= 320)
		{
			GlslVersion = "150";
		}
		else if (SupportedOpenGLLevel >= 310)
		{
			GlslVersion = "140";
		}
		else if (SupportedOpenGLLevel >= 300)
		{
			GlslVersion = "130";
		}
		else if (SupportedOpenGLLevel >= 210)
		{
			GlslVersion = "120";
		}
		else if (SupportedOpenGLLevel >= 200)
		{
			GlslVersion = "110";
		}
		else
		{
			fprintf(stderr, "Error: OpenGL Version %d.%d does not support shaders.\n", majorVersion, minorVersion);
			exit(1);
		}
	}

	std::string text = load_shader(file);

	std::string shaderText = "#version " + GlslVersion + "\n#define GLSL_VERSION " + GlslVersion + "\n" + text;

	add_program(text, type);

	//std::string attributeKeyword = "in";
	//add_all_attributes(vertexShaderText, attributeKeyword);

	compile_shader();

	add_shader_uniforms(text);
}

ShaderData::~ShaderData()
{
	for (std::vector<int>::iterator it = mShaders.begin(); it != mShaders.end(); ++it)
	{
		glDetachShader(mProgram, *it);
		glDeleteShader(*it);
	}
	glDeleteProgram(mProgram);
}

void ShaderData::add_program(const std::string& text, int type)
{
	int shader = glCreateShader(type);

	if (shader == 0)
	{
		fprintf(stderr, "Error creating shader type %d\n", type);
		exit(1);
	}

	const GLchar* p[1];
	p[0] = text.c_str();
	GLint lengths[1];
	lengths[0] = text.length();

	glShaderSource(shader, 1, p, lengths);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar InfoLog[1024];

		glGetShaderInfoLog(shader, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", shader, InfoLog);

		exit(1);
	}

	glAttachShader(mProgram, shader);
	mShaders.push_back(shader);
}

//void ShaderData::add_all_attributes(const std::string& vertexShaderText, const std::string& attributeKeyword)
//{
//	int currentAttribLocation = 0;
//	size_t attributeLocation = vertexShaderText.find(attributeKeyword);
//	while (attributeLocation != std::string::npos)
//	{
//		bool isCommented = false;
//		size_t lastLineEnd = vertexShaderText.rfind("\n", attributeLocation);
//
//		if (lastLineEnd != std::string::npos)
//		{
//			std::string potentialCommentSection = vertexShaderText.substr(lastLineEnd, attributeLocation - lastLineEnd);
//
//			//Potential false positives are both in comments, and in macros.
//			isCommented = potentialCommentSection.find("//") != std::string::npos || potentialCommentSection.find("#") != std::string::npos;
//		}
//
//		if (!isCommented)
//		{
//			size_t begin = attributeLocation + attributeKeyword.length();
//			size_t end = vertexShaderText.find(";", begin);
//
//			std::string attributeLine = vertexShaderText.substr(begin + 1, end - begin - 1);
//
//			begin = attributeLine.find(" ");
//			std::string attributeName = attributeLine.substr(begin + 1);
//
//			glbindAttribLocation(mProgram, currentAttribLocation, attributeName.c_str());
//			currentAttribLocation++;
//		}
//		attributeLocation = vertexShaderText.find(attributeKeyword, attributeLocation + attributeKeyword.length());
//	}
//}

void ShaderData::add_shader_uniforms(const std::string& shaderText)
{
	static const std::string UNIFORM_KEY = "uniform";

	std::vector<UniformStruct> structs = find_uniform_structs(shaderText);

	size_t uniformLocation = shaderText.find(UNIFORM_KEY);
	while (uniformLocation != std::string::npos)
	{
		bool isCommented = false;
		size_t lastLineEnd = shaderText.rfind("\n", uniformLocation);

		if (lastLineEnd != std::string::npos)
		{
			std::string potentialCommentSection = shaderText.substr(lastLineEnd, uniformLocation - lastLineEnd);
			isCommented = potentialCommentSection.find("//") != std::string::npos;
		}

		if (!isCommented)
		{
			size_t begin = uniformLocation + UNIFORM_KEY.length();
			size_t end = shaderText.find(";", begin);

			std::string uniformLine = shaderText.substr(begin + 1, end - begin - 1);

			begin = uniformLine.find(" ");
			std::string uniformName = uniformLine.substr(begin + 1);
			std::string uniformType = uniformLine.substr(0, begin);

			mUniformNames.push_back(uniformName);
			mUniformTypes.push_back(uniformType);
			add_uniform(uniformName, uniformType, structs);
		}
		uniformLocation = shaderText.find(UNIFORM_KEY, uniformLocation + UNIFORM_KEY.length());
	}
}

void ShaderData::add_uniform(const std::string& uniformName, const std::string& uniformType, const std::vector<UniformStruct>& structs)
{
	bool addThis = true;

	for (unsigned int i = 0; i < structs.size(); i++)
	{
		if (structs[i].get_name().compare(uniformType) == 0)
		{
			addThis = false;
			for (unsigned int j = 0; j < structs[i].get_membernames().size(); j++)
			{
				add_uniform(uniformName + "." + structs[i].get_membernames()[j].get_name(), structs[i].get_membernames()[j].get_type(), structs);
			}
		}
	}

	if (!addThis)
		return;

	unsigned int location = glGetUniformLocation(mProgram, uniformName.c_str());

	assert(location != INVALID_VALUE);

	mUniformMap.insert(std::pair<std::string, unsigned int>(uniformName, location));
}

void ShaderData::compile_shader() const
{
	glLinkProgram(mProgram);
	check_shader_error(mProgram, GL_LINK_STATUS, true, "Error linking shader program");

	glValidateProgram(mProgram);
	check_shader_error(mProgram, GL_VALIDATE_STATUS, true, "Invalid shader program");
}

//////////////////////////////////
///////////Shader/////////////////
//////////////////////////////////

Shader::Shader(const std::string& vsFile, const std::string& fsFile)
{
	mVsName = vsFile;
	mFsName = fsFile;
}

void Shader::init()
{
	std::map<std::string, ShaderData*>::const_iterator it = ResourceMap.find(mVsName + mFsName);
	if (it != ResourceMap.end())
	{
		mShaderData = it->second;
		mShaderData->add_reference();
	}
	else
	{
		mShaderData = new ShaderData();
		mShaderData->init(mVsName, GL_VERTEX_SHADER);
		mShaderData->init(mFsName, GL_FRAGMENT_SHADER);
		ResourceMap.insert(std::pair<std::string, ShaderData*>(mVsName + mFsName, mShaderData));
	}
}

Shader::Shader(const Shader& other) :
	mShaderData(other.mShaderData),
	mVsName(other.mVsName),
	mFsName(other.mFsName),
	mGeName(other.mGeName)
{
	mShaderData->add_reference();
}

Shader::~Shader()
{
	if (mShaderData && mShaderData->remove_reference())
	{
		if (mVsName.length() > 0)
			ResourceMap.erase(mVsName);

		delete mShaderData;
	}
}

Shader& Shader::operator=(const Shader &other)
{
	mShaderData = other.mShaderData; 
	mVsName = other.mVsName; 
	mFsName = other.mFsName;  
	mGeName = other.mGeName;
	mShaderData->add_reference();

	return *this;
}

void Shader::use() const
{
	glUseProgram(mShaderData->get_program());
}

//void Shader::update_uniforms(const Transform& transform, const Material& material, const RenderEngine& renderEngine, const Camera& camera) const
//{
//	glm::mat4 worldMatrix = transform.get_model();
//	glm::mat4 projectedMatrix = camera.get_view_projection() * worldMatrix;
//
//	for (unsigned int i = 0; i < mShaderData->get_uniform_names().size(); i++)
//	{
//		std::string uniformName = mShaderData->get_uniform_names()[i];
//		std::string uniformType = mShaderData->get_uniform_types()[i];
//
//		// RenderEngine variance  
//		if (uniformName.substr(0, 2) == "R_")
//		{
//			/*std::string unprefixedName = uniformName.substr(2, uniformName.length());
//
//			if (unprefixedName == "lightMatrix")
//				set_uniformmat4(uniformName, renderEngine.GetLightMatrix() * worldMatrix);
//			else if (uniformType == "sampler2D")
//			{
//				int samplerSlot = renderEngine.GetSamplerSlot(unprefixedName);
//				renderEngine.GetTexture(unprefixedName).bind(samplerSlot);
//				set_uniformi(uniformName, samplerSlot);
//			}
//			else if (uniformType == "vec3")
//				set_uniformvec3(uniformName, renderEngine.GetVector3f(unprefixedName));
//			else if (uniformType == "float")
//				set_uniformf(uniformName, renderEngine.GetFloat(unprefixedName));*/
//			/*else if (uniformType == "DirectionalLight")
//				set_uniform_dirlight(uniformName, *(const DirectionalLight*)&renderEngine.GetActiveLight());
//			else if (uniformType == "PointLight")
//				set_uniform_pointlight(uniformName, *(const PointLight*)&renderEngine.GetActiveLight());
//			else if (uniformType == "SpotLight")
//				set_uniform_spotlight(uniformName, *(const SpotLight*)&renderEngine.GetActiveLight());*/
//			/*else
//				renderEngine.update_uniformstruct(transform, material, *this, uniformName, uniformType);*/
//		}
//		//texture variance
//		else if (uniformType == "sampler2D")
//		{
//			int samplerSlot = renderEngine.get_sampler_slot(uniformName);
//			material.get_texture(uniformName).bind(samplerSlot);
//			set_int(uniformName, samplerSlot);
//		}
//		//transform variance
//		else if (uniformName.substr(0, 2) == "T_")
//		{
//			if (uniformName == "T_MVP")
//				set_mat4(uniformName, projectedMatrix);
//			else if (uniformName == "T_model")
//				set_mat4(uniformName, worldMatrix);
//			else
//				throw "Invalid Transform Uniform: " + uniformName;
//		}
//		//constant variance
//		else if (uniformName.substr(0, 2) == "C_")
//		{
//			//camera position
//			if (uniformName == "C_cameraPos")
//				set_vec3(uniformName, camera.get_transform().get_transform_pos());
//			else
//				throw "Invalid Camera Uniform: " + uniformName;
//		}
//		//other variance
//		else
//		{
//			if (uniformType == "vec3")
//				set_vec3(uniformName, material.get_vec3(uniformName));
//			else if (uniformType == "float")
//				set_float(uniformName, material.get_float(uniformName));
//			else
//				throw uniformType + " is not supported by the Material class";
//		}
//	}
//}

void Shader::set_int(const std::string& uniformName, int value) const
{
	glUniform1i(mShaderData->get_uniform_map().at(uniformName), value);
}

void Shader::set_float(const std::string& uniformName, float value) const
{
	glUniform1f(mShaderData->get_uniform_map().at(uniformName), value);
}

void Shader::set_vec3(const std::string& uniformName, const glm::vec3& value) const
{
	glUniform3f(mShaderData->get_uniform_map().at(uniformName), value.x, value.y, value.z);
}

void Shader::set_mat4(const std::string& uniformName, const glm::mat4& value) const
{
	glUniformMatrix4fv(mShaderData->get_uniform_map().at(uniformName), 1, GL_FALSE, &(value[0][0]));
}

bool Shader::is_default()
{
	return mVsName == "./shader/default.vert" && mFsName == "./shader/default.frag";
}

//void Shader::set_uniform_dirlight(const std::string& uniformName, const DirectionalLight& directionalLight) const
//{
//	set_uniformvec3(uniformName + ".direction", directionalLight.GetTransform().GetTransformedRot().GetForward());
//	set_uniformvec3(uniformName + ".base.color", directionalLight.GetColor());
//	set_uniformf(uniformName + ".base.intensity", directionalLight.GetIntensity());
//}
//
//void Shader::set_uniform_pointlight(const std::string& uniformName, const PointLight& pointLight) const
//{
//	set_uniformvec3(uniformName + ".base.color", pointLight.GetColor());
//	set_uniformf(uniformName + ".base.intensity", pointLight.GetIntensity());
//	set_uniformf(uniformName + ".atten.constant", pointLight.GetAttenuation().GetConstant());
//	set_uniformf(uniformName + ".atten.linear", pointLight.GetAttenuation().GetLinear());
//	set_uniformf(uniformName + ".atten.exponent", pointLight.GetAttenuation().GetExponent());
//	set_uniformvec3(uniformName + ".position", pointLight.GetTransform().GetTransformedPos());
//	set_uniformf(uniformName + ".range", pointLight.GetRange());
//}
//
//void Shader::set_uniform_spotlight(const std::string& uniformName, const SpotLight& spotLight) const
//{
//	set_uniformvec3(uniformName + ".pointLight.base.color", spotLight.GetColor());
//	set_uniformf(uniformName + ".pointLight.base.intensity", spotLight.GetIntensity());
//	set_uniformf(uniformName + ".pointLight.atten.constant", spotLight.GetAttenuation().GetConstant());
//	set_uniformf(uniformName + ".pointLight.atten.linear", spotLight.GetAttenuation().GetLinear());
//	set_uniformf(uniformName + ".pointLight.atten.exponent", spotLight.GetAttenuation().GetExponent());
//	set_uniformvec3(uniformName + ".pointLight.position", spotLight.GetTransform().GetTransformedPos());
//	set_uniformf(uniformName + ".pointLight.range", spotLight.GetRange());
//	set_uniformvec3(uniformName + ".direction", spotLight.GetTransform().GetTransformedRot().GetForward());
//	set_uniformf(uniformName + ".cutoff", spotLight.GetCutoff());
//}
