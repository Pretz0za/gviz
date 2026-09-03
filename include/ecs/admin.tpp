#pragma once

#include "admin.hpp"

template <typename T> ComponentPool<T> &Admin::GetPool() {}

template <typename T> T &Admin::AddComponent(EntityID id) {}

template <typename T> T *Admin::GetComponent(EntityID id) {}

template <typename T> bool Admin::HasComponent(EntityID id) {}
