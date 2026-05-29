#include "CameraManager.h"
#include "Logger.h"

void CameraManager::Initialize() {
    defaultCamera_ = std::make_shared<Camera>(Camera::ConstructorKey());
    activeCamera_ = defaultCamera_.get();
}

void CameraManager::Finalize() {
    defaultCamera_.reset();
	cameraList_.clear();
	debugCameraList_.clear();
	activeCamera_ = nullptr;
}

std::weak_ptr<Camera> CameraManager::CreateCamera() {
	cameraList_.push_back(std::make_shared<Camera>(Camera::ConstructorKey()));
    return cameraList_.back();
}

std::weak_ptr<DebugCamera> CameraManager::CreateDebugCamera(kEngine* kEngine) {
    debugCameraList_.push_back(std::make_shared<DebugCamera>(kEngine, DebugCamera::ConstructorKey()));
    return debugCameraList_.back();
}

void CameraManager::DestroyCamera(std::weak_ptr<Camera> cam) {

    auto sp = cam.lock();
    if (!sp) { 
		Logger::Log("kEngine: CameraManager::DestroyCamera: Camera pointer is expired.");
        return; 
    }

    auto target = std::find_if(cameraList_.begin(), cameraList_.end(),
        [&](const std::shared_ptr<Camera>& c) {
            return c.get() == sp.get();
        });

    if(target != cameraList_.end()) {
        if (activeCamera_ == target->get()) {
            ResetActiveCamera();
        }
        cameraList_.erase(target);
    } else {

        auto debugTarget = std::find_if(debugCameraList_.begin(), debugCameraList_.end(),
            [&](const std::shared_ptr<DebugCamera>& c) {
                return c.get() == sp.get();
            });
        if(debugTarget != debugCameraList_.end()) {
            if (activeCamera_ == debugTarget->get()) {
                ResetActiveCamera();
            }
            debugCameraList_.erase(debugTarget);
		}
    }
}

void CameraManager::SetActiveCamera(std::weak_ptr<Camera> cam) {

    auto sp = cam.lock();
    if (!sp) {
		Logger::Log("kEngine: CameraManager::SetActiveCamera: Camera pointer is expired.");
        return;
    }

    auto target = std::find_if(cameraList_.begin(), cameraList_.end(),
        [&](const std::shared_ptr<Camera>& c) {
            return c.get() == sp.get();
        });
    if(target != cameraList_.end()) {
        activeCamera_ = target->get();
    } else {
        auto debugTarget = std::find_if(debugCameraList_.begin(), debugCameraList_.end(),
            [&](const std::shared_ptr<DebugCamera>& c) {
                return c.get() == sp.get();
            });
        if (debugTarget != debugCameraList_.end()) {
            activeCamera_ = debugTarget->get();
        } else {
			Logger::Log("kEngine: CameraManager:: Camera not found. Resetting to default camera.");
        }
    }
}

void CameraManager::ResetActiveCamera() {
    activeCamera_ = defaultCamera_.get();
}

Camera* CameraManager::GetActiveCamera() const {
    return activeCamera_ ? activeCamera_ : defaultCamera_.get();
}
