#pragma once

#include "CameraComponent.h"

#include "Core/Math/CameraMath.h"

namespace Murder
{
    struct CameraUpdateParams
    {
        float forward = 0.0f;
        float right   = 0.0f;
        float up      = 0.0f;

        float pitchDelta = 0.0f;
        float yawDelta   = 0.0f;

        bool fast = false; // for sprinting or something
        bool slow = false; // for precision movement
    };

    class ICameraControllerCpu
    {
      public:
        virtual ~ICameraControllerCpu()                                                    = default;
        virtual void Update(Camera& camera, const CameraUpdateParams& params, float dt) = 0;
    };

    class FpsCameraControllerCpu final : public ICameraControllerCpu
    {
      public:
        // member variables are kept public for easy tweaking
        // TODO: consider adding getters and setters

        float moveSpeed      = 5.0f;    // units per second
        float fastMultiplier = 2.0f;    // units per second when fast is true
        float slowMultiplier = 0.25f;   // units per second when slow is true
        float sensitivity    = 0.0025f; // radians per pixel

        bool  invertY        = false;
        bool  constrainPitch = true;                 // to avoid gimbal lock and rolling
        float pitchMax = DirectX::XM_PIDIV2 - 0.01f; // max pitch angle, just under 90 degrees to avoid gimbal lock
        float pitchMin = -pitchMax;                  // min pitch angle

        bool grounded = true; // only xz movement when grounded (fps)
      public:
        void LoadCamera(const Camera& camera)
        {
            // extract yaw and pitch from camera rotation
            Vector3 forward = CameraMath::NormalizeSafe(camera.Forward(), Vector3::UnitZ);

            yaw   = std::atan2(forward.x, forward.z);
            pitch = std::asin(std::clamp(forward.y, -1.0f, 1.0f));
        }

        void Update(Camera& camera, const CameraUpdateParams& params, float dt) override
        {
            // Look
            const float pitchSign = invertY ? -1.0f : 1.0f;

            yaw   += params.yawDelta * sensitivity;
            pitch += params.pitchDelta * sensitivity * pitchSign;

            if (constrainPitch)
                pitch = std::clamp(pitch, pitchMin, pitchMax);

            camera.rotation = Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0.0f);

            // Move
            Vector3 forward = camera.Forward();
            Vector3 right   = camera.Right();
            Vector3 up      = camera.worldUp;

            if (grounded)
            {
                forward -= up * forward.Dot(up);
                right   -= up * right.Dot(up);

                forward = CameraMath::NormalizeSafe(forward, Vector3::UnitZ);
                right   = CameraMath::NormalizeSafe(right, Vector3::UnitX);
            }

            Vector3 move = (forward * params.forward) + (right * params.right);
            if (!grounded)
                move += up * params.up;

            move = CameraMath::NormalizeSafe(move, Vector3::Zero);

            float speed = moveSpeed;
            if (params.fast)
                speed *= fastMultiplier;
            if (params.slow)
                speed *= slowMultiplier;

            camera.position += move * speed * dt;
        }

        float Yaw() const { return yaw; }
        float Pitch() const { return pitch; }

      private:
        float yaw   = 0.0f; // radians
        float pitch = 0.0f; // radians
    };

    class OrbitCameraControllerCpu final : public ICameraControllerCpu
    {
      public:
        void Update(Camera& camera, const CameraUpdateParams& params, float dt) override
        {
            yaw += params.yawDelta * lookSpeed +
                   params.right; // allow strafing to also rotate the camera around the target
            pitch += params.pitchDelta * lookSpeed;

            if (constrainPitch)
                pitch = std::clamp(pitch, pitchMin, pitchMax);

            // zoom / dolly in-out
            radius -= params.forward * dt * 5.0f;
            radius  = (std::max)(radius, 0.05f);

            // Build orbit pose directly from yaw/pitch to avoid LookAt pole singularities.
            camera.rotation = Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0.0f);

            const Vector3 orbitDir = CameraMath::NormalizeSafe(camera.Forward(), Vector3::UnitZ);
            camera.position        = target - orbitDir * radius;
        }

        // orbit params
        Vector3 target    = { 0, 0, 0 }; // point to orbit around
        float   radius    = 5.0f;        // distance from target
        float   lookSpeed = 0.1f;        // radians per pixel
        float   zoomSpeed = 2.0f;        // world units per second

        bool  constrainPitch = true;
        float pitchMax       = DirectX::XM_PIDIV2 - 0.01f;
        float pitchMin       = -pitchMax;

      private:
        float yaw   = 0.0f; // radians
        float pitch = 0.0f; // radians
    };

} // namespace Murder
