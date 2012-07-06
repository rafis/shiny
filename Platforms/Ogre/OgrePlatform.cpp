#include "OgrePlatform.hpp"

#include <OgreDataStream.h>
#include <OgreGpuProgramManager.h>
#include <OgreRoot.h>

#include "OgreMaterial.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreMaterialSerializer.hpp"

#include "../../Main/MaterialInstance.hpp"
#include "../../Main/Factory.hpp"

namespace
{
	std::string convertLang (sh::Language lang)
	{
		if (lang == sh::Language_CG)
			return "cg";
		else if (lang == sh::Language_HLSL)
			return "hlsl";
		else if (lang == sh::Language_GLSL)
			return "glsl";
		throw std::runtime_error ("invalid language, valid are: cg, hlsl, glsl");
	}
}

namespace sh
{
	OgreMaterialSerializer* OgrePlatform::sSerializer = 0;

	OgrePlatform::OgrePlatform(const std::string& resourceGroupName, const std::string& basePath)
		: Platform(basePath)
		, mResourceGroup(resourceGroupName)
	{
		Ogre::MaterialManager::getSingleton().addListener(this);

		Ogre::GpuProgramManager::getSingletonPtr()->setSaveMicrocodesToCache(true);

		sSerializer = new OgreMaterialSerializer();
	}

	OgreMaterialSerializer& OgrePlatform::getSerializer()
	{
		assert(sSerializer);
		return *sSerializer;
	}

	Language OgrePlatform::selectBestLanguage ()
	{
		std::string name = Ogre::Root::getSingleton().getRenderSystem ()->getName();
		if (name.find("OpenGL") != std::string::npos)
			return Language_GLSL;
		else if (name.find("Direct3D") != std::string::npos)
			return Language_HLSL;
		else
			return Language_CG;
	}

	OgrePlatform::~OgrePlatform ()
	{
		delete sSerializer;
	}

	bool OgrePlatform::isProfileSupported (const std::string& profile)
	{
		return Ogre::GpuProgramManager::getSingleton().isSyntaxSupported(profile);
	}

	bool OgrePlatform::supportsShaderSerialization ()
	{
		// not working in GLSL for some reason, ogre bug?
		return (mFactory->getCurrentLanguage() != Language_GLSL) && mShaderCachingEnabled;
	}

	bool OgrePlatform::supportsMaterialQueuedListener ()
	{
		return true;
	}

	boost::shared_ptr<Material> OgrePlatform::createMaterial (const std::string& name)
	{
		OgreMaterial* material = new OgreMaterial(name, mResourceGroup);
		return boost::shared_ptr<Material> (material);
	}

	boost::shared_ptr<GpuProgram> OgrePlatform::createGpuProgram (
		GpuProgramType type,
		const std::string& compileArguments,
		const std::string& name, const std::string& profile,
		const std::string& source, Language lang)
	{
		OgreGpuProgram* prog = new OgreGpuProgram (type, compileArguments, name, profile, source, convertLang(lang), mResourceGroup);
		return boost::shared_ptr<GpuProgram> (static_cast<GpuProgram*>(prog));
	}

	Ogre::Technique* OgrePlatform::handleSchemeNotFound (
		unsigned short schemeIndex, const Ogre::String &schemeName, Ogre::Material *originalMaterial,
		unsigned short lodIndex, const Ogre::Renderable *rend)
	{
		MaterialInstance* m = fireMaterialRequested(originalMaterial->getName(), schemeName);
		if (m)
		{
			OgreMaterial* _m = static_cast<OgreMaterial*>(m->getMaterial());
			return _m->getOgreTechniqueForConfiguration (schemeName);
		}
		else
			return 0; // material does not belong to us
	}

	void OgrePlatform::serializeShaders (const std::string& file)
	{
		std::fstream output;
		output.open(file.c_str(), std::ios::out | std::ios::binary);
		Ogre::DataStreamPtr shaderCache (OGRE_NEW Ogre::FileStreamDataStream(file, &output, false));
		Ogre::GpuProgramManager::getSingleton().saveMicrocodeCache(shaderCache);
	}

	void OgrePlatform::deserializeShaders (const std::string& file)
	{
		std::ifstream inp;
		inp.open(file.c_str(), std::ios::in | std::ios::binary);
		Ogre::DataStreamPtr shaderCache(OGRE_NEW Ogre::FileStreamDataStream(file, &inp, false));
		Ogre::GpuProgramManager::getSingleton().loadMicrocodeCache(shaderCache);
	}

	void OgrePlatform::setSharedParameter (const std::string& name, PropertyValuePtr value)
	{
		Ogre::GpuSharedParametersPtr params;
		if (mSharedParameters.find(name) == mSharedParameters.end())
		{
			params = Ogre::GpuProgramManager::getSingleton().createSharedParameters(name);
			params->addConstantDefinition(name, Ogre::GCT_FLOAT4); //always use float4 for now
			mSharedParameters[name] = params;
		}
		else
			params = mSharedParameters.find(name)->second;

		/// \todo remove the typeid and pass it instead (ValueType enum)
		Ogre::Vector4 v (1.0, 1.0, 1.0, 1.0);
		if (typeid(*value) == typeid(Vector4))
		{
			Vector4 vec = retrieveValue<Vector4>(value, NULL);
			v.x = vec.mX;
			v.y = vec.mY;
			v.z = vec.mZ;
			v.w = vec.mW;
		}
		else if (typeid(*value) == typeid(Vector3))
		{
			Vector3 vec = retrieveValue<Vector3>(value, NULL);
			v.x = vec.mX;
			v.y = vec.mY;
			v.z = vec.mZ;
		}
		else if (typeid(*value) == typeid(Vector2))
		{
			Vector2 vec = retrieveValue<Vector2>(value, NULL);
			v.x = vec.mX;
			v.y = vec.mY;
		}
		else if (typeid(*value) == typeid(FloatValue))
			v.x = retrieveValue<FloatValue>(value, NULL).get();
		else if (typeid(*value) == typeid(IntValue))
			v.x = static_cast<float>(retrieveValue<IntValue>(value, NULL).get());
		else
			throw std::runtime_error ("unsupported property type for shared parameter \"" + name + "\"");
		params->setNamedConstant(name, v);
	}
}
