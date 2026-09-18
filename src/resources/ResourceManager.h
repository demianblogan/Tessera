#pragma once

#include <filesystem>
#include <unordered_map>
#include <stdexcept>
#include <utility>

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "Assets.h"

template <typename Resource, typename Identifier>
class ResourceManager
{
public:
	// `loadArgs` is forwarded straight to openFromFile/loadFromFile after the
	// path -- empty for Music/Font/Texture/SoundBuffer, one sf::Shader::Type
	// for Shader. Lets every resource type share this one Load() instead of
	// each needing its own near-identical manager.
	template <typename... LoadArgs>
	void Load(Identifier id, const std::filesystem::path& filepath, LoadArgs&&... loadArgs)
	{
		static_assert(std::default_initializable<Resource>);

		Resource resource;
		bool isResourceLoaded = false;

		if constexpr (std::is_same_v<Resource, sf::Music> || std::is_same_v<Resource, sf::Font>)
			isResourceLoaded = resource.openFromFile(filepath, std::forward<LoadArgs>(loadArgs)...);
		else
			isResourceLoaded = resource.loadFromFile(filepath, std::forward<LoadArgs>(loadArgs)...);

		if (!isResourceLoaded)
			throw std::runtime_error("Failed to load resource: " + filepath.string());

		const auto [_, isResourceInserted] = resources.emplace(id, std::move(resource));
		if (!isResourceInserted)
		{
			throw std::runtime_error("Resource already loaded.");
		}
	}

	[[nodiscard]] bool Contains(Identifier id) const
	{
		return resources.contains(id);
	}

	Resource& Get(Identifier id)
	{
		if (!resources.contains(id))
			throw std::runtime_error("Requested resource was not loaded.");

		return resources.at(id);
	}

	const Resource& Get(Identifier id) const
	{
		if (!resources.contains(id))
			throw std::runtime_error("Requested resource was not loaded.");

		return resources.at(id);
	}

	template <typename Function>
	void ForEach(Function function)
	{
		for (auto& [id, resource] : resources)
			function(resource);
	}

private:
	std::unordered_map<Identifier, Resource> resources;
};

using FontManager = ResourceManager<sf::Font, Assets::FontID>;
using MusicManager = ResourceManager<sf::Music, Assets::MusicID>;
using SoundBufferManager = ResourceManager<sf::SoundBuffer, Assets::SoundID>;
using TextureManager = ResourceManager<sf::Texture, Assets::TextureID>;
using ShaderManager = ResourceManager<sf::Shader, Assets::ShaderID>;
