#include "MaterialInstance.hpp"

#include "Factory.hpp"
#include "ShaderSet.hpp"

namespace sh
{
	MaterialInstance::MaterialInstance (const std::string& name, Factory* f)
		: mName(name)
		, mShadersEnabled(true)
		, mFactory(f)
	{
	}

	void MaterialInstance::setParentInstance (const std::string& name)
	{
		mParentInstance = name;
	}

	std::string MaterialInstance::getParentInstance ()
	{
		return mParentInstance;
	}

	void MaterialInstance::create (Platform* platform)
	{
		mMaterial = platform->createMaterial(mName);
	}

	void MaterialInstance::destroyAll ()
	{
		mMaterial->removeAll();
	}

	void MaterialInstance::setProperty (const std::string& name, PropertyValuePtr value)
	{
		PropertySetGet::setProperty (name, value);
		if (mMaterial)
			mMaterial->removeAll(); // trigger updates
	}

	void MaterialInstance::createForConfiguration (Platform* platform, const std::string& configuration)
	{
		mMaterial->createConfiguration(configuration);

		// get passes of the top-most parent
		PassVector passes = getPasses();
		for (PassVector::iterator it = passes.begin(); it != passes.end(); ++it)
		{
			boost::shared_ptr<Pass> pass = mMaterial->createPass (configuration);
			it->copyAll (pass.get(), this);

			std::vector<std::string> usedTextureSamplers; // texture samplers used in the shaders

			// create or retrieve shaders
			if (mShadersEnabled)
			{
				it->setContext(this);
				ShaderSet* vertex = mFactory->getShaderSet(retrieveValue<StringValue>(it->getProperty("vertex_program"), this).get());
				ShaderInstance* v = vertex->getInstance(&*it);
				if (v)
				{
					pass->assignProgram (GPT_Vertex, v->getName());
					v->setUniformParameters (pass, &*it);
					std::vector<std::string> vector = v->getUsedSamplers ();
					usedTextureSamplers.insert(usedTextureSamplers.end(), vector.begin(), vector.end());
				}
				ShaderSet* fragment = mFactory->getShaderSet(retrieveValue<StringValue>(it->getProperty("fragment_program"), this).get());
				ShaderInstance* f = fragment->getInstance(&*it);
				if (f)
				{
					pass->assignProgram (GPT_Fragment, f->getName());
					f->setUniformParameters (pass, &*it);
					std::vector<std::string> vector = f->getUsedSamplers ();
					usedTextureSamplers.insert(usedTextureSamplers.end(), vector.begin(), vector.end());
				}
			}

			// create texture units
			std::map<std::string, MaterialInstanceTextureUnit> texUnits = it->getTexUnits();
			for (std::map<std::string, MaterialInstanceTextureUnit>::iterator texIt = texUnits.begin(); texIt  != texUnits.end(); ++texIt )
			{
				// only create those that are needed by the shader, OR those marked to be created in fixed function pipeline if shaders are disabled
				if (std::find(usedTextureSamplers.begin(), usedTextureSamplers.end(), texIt->second.getName()) != usedTextureSamplers.end()
						|| (!mShadersEnabled && texIt->second.hasProperty("create_in_ffp") && retrieveValue<BooleanValue>(texIt->second.getProperty("create_in_ffp"), this).get()))
				{
					boost::shared_ptr<TextureUnitState> texUnit = pass->createTextureUnitState ();
					texIt->second.copyAll (texUnit.get(), this);
				}
			}
		}
	}

	void MaterialInstance::markDirty (const std::string& configuration)
	{
		mMaterial->removeConfiguration(configuration);
	}

	Material* MaterialInstance::getMaterial ()
	{
		return mMaterial.get();
	}

	MaterialInstancePass* MaterialInstance::createPass ()
	{
		mPasses.push_back (MaterialInstancePass());
		mPasses.back().setContext(this);
		return &mPasses.back();
	}

	PassVector MaterialInstance::getPasses()
	{
		if (mParent)
			return static_cast<MaterialInstance*>(mParent)->getPasses();
		else
			return mPasses;
	}

	void MaterialInstance::setShadersEnabled (bool enabled)
	{
		if (enabled == mShadersEnabled)
			return;
		mShadersEnabled = enabled;

		// trigger updates
		if (mMaterial.get())
			mMaterial->removeAll();
	}
}
