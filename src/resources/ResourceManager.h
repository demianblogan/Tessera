#pragma once

#include <filesystem>
#include <unordered_map>
#include <stdexcept>

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "Assets.h"

template <typename Resource, typename Identifier>
class ResourceManager
{
private:
	std::unordered_map<Identifier, Resource> resources;

public:
	void Load(Identifier id, const std::filesystem::path& filepath)
	{
		static_assert(std::default_initializable<Resource>);

		Resource resource;
		bool resourceLoaded = false;

		if constexpr (std::is_same_v<Resource, sf::Music> || std::is_same_v<Resource, sf::Font>)
		{
			resourceLoaded = resource.openFromFile(filepath);
		}
		else
		{
			resourceLoaded = resource.loadFromFile(filepath);
		}

		if (!resourceLoaded)
		{
			throw std::runtime_error("Failed to load resource: " + filepath.string());
		}

		const auto [_, inserted] = resources.emplace(id, std::move(resource));
		if (!inserted)
		{
			throw std::runtime_error("Resource already loaded.");
		}
	}

	Resource& Get(Identifier id)
	{
		if (!resources.contains(id))
		{
			throw std::runtime_error("Requested resource was not loaded.");
		}

		return resources.at(id);
	}

	const Resource& Get(Identifier id) const
	{
		if (!resources.contains(id))
		{
			throw std::runtime_error("Requested resource was not loaded.");
		}

		return resources.at(id);
	}

	template <typename Function>
	void ForEach(Function function)
	{
		for (auto& [id, resource] : resources)
		{
			function(resource);
		}
	}
};

using FontManager = ResourceManager<sf::Font, Assets::FontID>;
using MusicManager = ResourceManager<sf::Music, Assets::MusicID>;
using SoundBufferManager = ResourceManager<sf::SoundBuffer, Assets::SoundID>;
using TextureManager = ResourceManager<sf::Texture, Assets::TextureID>;