#pragma once

// my libs
#include "rap_model.hpp"

#include <glm/gtc/matrix_transform.hpp>

// std
#include <memory>
#include <unordered_map>

namespace rap {

    struct TransformComponent {
        glm::vec3 translation{};
        glm::vec3 scale{ 1.f, 1.f, 1.f };
        glm::vec3 rotation{};

        // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
        // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
        // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
        glm::mat4 mat4();

        glm::mat3 normalMatrix();
    };

    struct PointLightComponent {
        float lightIntensity = 1.f;

    };

    class RaptorGameObject {
    public:
        using id_t = unsigned int;
		using Map = std::unordered_map<id_t, RaptorGameObject>;

        static RaptorGameObject createGameObject() {
            static id_t currentId = 0;
            return RaptorGameObject{ currentId++ };
        }

        static RaptorGameObject makePointLight(float intensity = 1.f, float radius = 0.1, glm::vec3 color = glm::vec3{1.f});

        RaptorGameObject(const RaptorGameObject&) = delete;
        RaptorGameObject& operator=(const RaptorGameObject&) = delete;
        RaptorGameObject(RaptorGameObject&&) = default;
        RaptorGameObject& operator=(RaptorGameObject&&) = default;

        id_t getId() const {
            return id;
        }

        glm::vec3 color{};
        TransformComponent transform{};

        //mmmm yes ptrs very optional tho
        std::shared_ptr<RaptorModel> model{};
        std::unique_ptr<PointLightComponent> pointLight = nullptr;

    private:
        RaptorGameObject(id_t objId) : id{ objId } {}

        id_t id;
    };

} // namespace rap