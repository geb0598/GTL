# 🚀 최적화 게임잼 하이라이트

우리 엔진이 120fps를 유지하면서도 에디터 조작(예: 피킹)이 부드럽게 돌아가도록 만든 핵심 최적화들을 정리했습니다.

---

## 🎯 목표

- 📈 무거운 씬에서도 에디터 프레임 레이트 향상
- 🎯 레이 피킹 비용을 쿼리당 0.01ms 이하로 단축
- 🧪 게임잼 진행 중에도 워크플로가 안정적으로 유지되도록 관리

---

## 🧭 씬 탐색 & 선택

### 🌳 계층형 피킹

- ✅ **월드 BVH**: 프리미티브 전역에 동적 BVH를 구축해 레이 쿼리의 광범위 탐색을 가속.
- 🧱 **삼각형 BVH**: 각 스태틱 메시는 삼각형 단위 계층을 사전 구축하여 리프에서만 정밀 테스트.
- 🔄 **로컬 레이 테스트**: 월드→모델 변환과 메시 포인터를 캐싱해, 히트마다 행렬 곱 없이 모델 공간에서 테스트 수행.

#### 🔧 핵심 코드 스니펫

```cpp
struct FBVHPrimitive
{
    FVector Center;
    FAABB Bounds;
    FMatrix WorldToModel;
    UPrimitiveComponent* Primitive = nullptr;
    EPrimitiveType PrimitiveType = EPrimitiveType::Cube;
    UStaticMesh* StaticMesh = nullptr;
};
```

### 🔍 프러스텀 컬링

- ✂️ 오브젝트 경계를 뷰 프러스텀과 먼저 교차 테스트.
- 🪟 렌더러와 BVH가 실제로 볼 수 있는 프리미티브만 처리.

### 🧱 오클루전 컬링

- 🧊 깊이 히어라키/오클루전 버퍼로 가려진 지오메트리를 조기 배제.
- ⛏️ 드로우 콜과 BVH가 탐색해야 할 프리미티브 수를 동시에 감축.

#### 🔧 핵심 코드 스니펫

```cpp
bool UBVHManager::Raycast(const FRay& InRay, UPrimitiveComponent*& HitComponent, float& HitT) const
{
    HitComponent = nullptr;
    if (RootIndex < 0 || Nodes.empty())
    {
        return false;
    }

    HitT = FLT_MAX;
    int hitIndex = -1;

    RaycastIterative(InRay, HitT, hitIndex);
    if (hitIndex == -1)
    {
        return false;
    }

    HitComponent = Primitives[hitIndex].Primitive;
    return true;
}
```

---

## 🧮 지오메트리 LOD

### 🔻 QEM(Quadratic Error Metric)

- 🧠 QEM 기반 메시 단순화로 LOD 생성.
- 🔀 거리 기반 LOD 전환으로 최소한의 이질감만으로 버텍스 수를 크게 감소.
- 📉 렌더링과 충돌/피킹 모두에서 거리 기준으로 저해상도 메시를 활용.

#### 🔧 핵심 코드 스니펫

```cpp
void FQemSimplifier::BuildLodChain(float targetError)
{
    while (!Heap.empty() && CurrentError() < targetError)
    {
        const FCollapseEdge edge = Heap.pop();
        if (IsValid(edge))
        {
            Collapse(edge);
        }
    }
}
```

---

## 🧵 멀티스레드 파이프라인

- 🧵 컬링, BVH 리핏, 스트리밍을 작업 단위로 나눠 병렬 처리.
- 📦 버퍼 업로드, 메시 BVH 빌드, 삼각형 쿼리를 가능한 범위에서 워커 스레드로 이전.
- 🔄 메인 스레드는 프레임 표시와 입력 처리에 집중하도록 유지.

#### 🔧 핵심 코드 스니펫

```cpp
ThreadPool.Enqueue([this]()
{
    FBVHBuildTask task;
    while (BuildQueue.TryPop(task))
    {
        task.Execute();
    }
});
```

---

## 🧠 수학 & SIMD

- 🌀 **SIMD 범용 적용**: 레이/AABB, 벡터 연산(내적·외적·길이), 삼각형 교차 등 핵심 수학 루틴 최적화.
- ❌ **행렬 곱 최소화**: 프리미티브별 월드→모델 변환을 캐싱하고, 레이를 한 번만 변환한 뒤 재활용.

#### 🔧 핵심 코드 스니펫

```cpp
FORCEINLINE bool FAABB::RaycastHit(const FRay& Ray, float* OutDistance) const
{
    __m128 origin = _mm_set_ps(0.f, Ray.Origin.Z, Ray.Origin.Y, Ray.Origin.X);
    __m128 dir    = _mm_set_ps(0.f, Ray.Direction.Z, Ray.Direction.Y, Ray.Direction.X);
    __m128 minv   = _mm_set_ps(0.f, Min.Z, Min.Y, Min.X);
    __m128 maxv   = _mm_set_ps(0.f, Max.Z, Max.Y, Max.X);

    __m128 invDir = _mm_div_ps(_mm_set1_ps(1.0f), dir);
    __m128 t1     = _mm_mul_ps(_mm_sub_ps(minv, origin), invDir);
    __m128 t2     = _mm_mul_ps(_mm_sub_ps(maxv, origin), invDir);

    __m128 tmin = _mm_min_ps(t1, t2);
    __m128 tmax = _mm_max_ps(t1, t2);

    float near = std::max(std::max(tmin.m128_f32[0], tmin.m128_f32[1]), tmin.m128_f32[2]);
    float far  = std::min(std::min(tmax.m128_f32[0], tmax.m128_f32[1]), tmax.m128_f32[2]);

    if (near > far || far < 0.f)
    {
        return false;
    }

    if (OutDistance)
    {
        *OutDistance = near > 0.f ? near : far;
    }
    return true;
}
```

---

## 🔧 기타 미세 조정

- 🔄 드래그 후 BVH 리핏으로 전체 재빌드 없이 바운드를 최신 상태로 유지.
- 🗂️ TArray 참조와 const-correct API로 불필요한 복사를 제거.
- 🧹 정렬된 배열·짧은 스택 등 캐시 친화적 자료구조로 트래버설 효율 향상.

---

## 📊 결과 스냅샷

- 🕹️ **피킹**: 캐시 적용 후 스태틱 메시 피킹이 ~0.007ms 수준으로 단축.
- 🖥️ **렌더 루프**: 컬링 + LOD + 멀티스레드 조합으로 안정적인 프레임 타임 확보.
- 🔁 **작업 효율**: 대규모 씬에서도 에디터 응답성 유지.

---

## 🗺️ 다음 단계

- 🧪 SIMD가 집중된 루틴에 대한 추가 프로파일링.
- 🧩 비메시 프리미티브를 위한 하이브리드 히트 테스트 개선.
- ⚙️ I/O 및 빌드를 겹칠 수 있도록 멀티스레드 스케줄링 재조정.

---

✨ 게임잼에서 함께 해 주셔서 감사합니다! 앞으로도 엔진을 계속 발전시켜 나갑시다.
