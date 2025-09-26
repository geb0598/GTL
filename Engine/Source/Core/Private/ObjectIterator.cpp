#include "pch.h"

#include "Core/Public/ObjectIterator.h"

// ObjectCacheManager 정적 변수 정의
std::unordered_map<std::type_index, TArray<UObject*>> ObjectCacheManager::ClassObjectCache;
bool ObjectCacheManager::bCacheValid = false;
size_t ObjectCacheManager::LastProcessedIndex = 0;

