#pragma once
#ifndef ANGKASA1_HASH_MAP
#define ANGKASA1_HASH_MAP

#include "Library/Algorithms/Hash.h"
#include "Pair.h"
#include "Array.h"

namespace astl
{
  namespace libenum
  {
    enum class BucketStatus : uint8
    {
      Empty,
      Occupied,
      Deleted
    };
  }

  template <typename K, typename V, typename UintWidth>
  class BaseMap
  {
  protected:

    using width_t = UintWidth;

    struct Bucket final : public Pair<K, V>
    {
      using Type = Pair<K, V>;
      libenum::BucketStatus Status;
      Bucket* Next;
      Bucket* Previous;

      using Pair<K, V>::Pair;

      Bucket() :
        Pair<K,V>{}, Status{ libenum::BucketStatus::Empty }, Next{}, Previous{}
      {}
      ~Bucket() { Status = libenum::BucketStatus::Empty; Next = Previous = nullptr; }
      Bucket(const Bucket& Rhs) { *this = Rhs; }
      Bucket(Bucket&& Rhs) { *this = Move(Rhs); }
      Bucket& operator=(const Bucket& Rhs)
      {
        if (this != &Rhs)
        {
          Status = Rhs.Status;
          Next = Rhs.Next;
          Previous = Rhs.Previous;
          Pair<K, V>::operator=(Rhs);
        }
        return *this;
      }
      Bucket& operator=(Bucket&& Rhs)
      {
        if (this != &Rhs)
        {
          Status = Rhs.Status;
          Next = Rhs.Next;
          Previous = Rhs.Previous;
          Pair<K, V>::operator=(Move(Rhs));
          new (&Rhs) Bucket();
        }
        return *this;
      }
    };

    Bucket* Head;
    Bucket* Tail;
    width_t NumBuckets;

    BaseMap() :
      Head{}, Tail{}, NumBuckets{}
    {}
    ~BaseMap()
    {
      Head = Tail = nullptr;
      NumBuckets = 0;
    }
    BaseMap(const BaseMap& Rhs) { *this = Rhs; }
    BaseMap(BaseMap&& Rhs) { *this = Move(Rhs); }
    BaseMap& operator=(const BaseMap& Rhs)
    {
      if (this != &Rhs)
      {
        Head = Rhs.Head;
        Tail = Rhs.Tail;
        NumBuckets = Rhs.NumBuckets;
      }
      return *this;
    }
    BaseMap& operator=(BaseMap&& Rhs)
    {
      if (this != &Rhs)
      {
        Head = Rhs.Head;
        Tail = Rhs.Tail;
        NumBuckets = Move(Rhs.NumBuckets);
        new (&Rhs) BaseMap();
      }
      return *this;
    }

    size_t Probe(size_t x, size_t b = 1, size_t a = 0)
    {
      return a * (x * x) + b * x;
    }

    size_t GetHashIndex(size_t Hash, size_t x, size_t Capacity)
    {
      return (Hash + Probe(x)) % Capacity;
    }

    void UpdateBucketChain(Bucket& Bckt)
    {
      if (Bckt.Previous) { Bckt.Previous->Next = &Bckt; }
      if (Bckt.Next) { Bckt.Next->Previous = &Bckt; }
      if (&Bckt == Head) { Head = &Bckt; }
      if (&Bckt == Tail) { Tail = &Bckt; }
    }

  public:
    using TBucket = Bucket;
  };

  template <typename K, typename V, typename Hasher, typename Container, typename UintWidth>
  class MapImpl final : public BaseMap<K, V, UintWidth>
  {
  private:
    using width_t = UintWidth;
    using TBucket = typename BaseMap<K, V, width_t>::Bucket;

    using Iterator = HashMapIterator<TBucket>;
    using ConstIterator = HashMapIterator<const TBucket>;
    using ReverseIterator = ReverseHashMapIterator<TBucket>;
    using ConstReverseIterator = ReverseHashMapIterator<const TBucket>;

    Container Entries;

    using BaseMap<K, V, width_t>::Head;
    using BaseMap<K, V, width_t>::Tail;
    using BaseMap<K, V, width_t>::NumBuckets;
    using BaseMap<K, V, width_t>::GetHashIndex;
    using BaseMap<K, V, width_t>::UpdateBucketChain;


    TBucket* FindBucketWithKey(const K& Key, bool Shift = true)
    {
      const width_t capacity = Entries.Size();

      if (!capacity) { return nullptr; }

      width_t hash = static_cast<width_t>(Hasher()(Key));
      width_t shiftIndex = -1;
      width_t constant = 0;
      width_t index = GetHashIndex(hash, constant++, capacity);
      width_t count = 0;
      TBucket* bucket = &Entries[index];

      while (bucket->Key != Key)
      {
        if (count == capacity) { return nullptr; }
        if (bucket->Status == libenum::BucketStatus::Deleted && shiftIndex == -1)
        {
          shiftIndex = index;
        }
        index = GetHashIndex(hash, constant++, capacity);
        bucket = &Entries[index];
        count++;
      }

      if (shiftIndex != -1 && Shift)
      {
        Entries[shiftIndex] = Move(*bucket);
        UpdateBucketChain(Entries[shiftIndex]);
        bucket = &Entries[shiftIndex];
      }
      return bucket;
    }

    void RemoveBucketWithKey(const K& Key)
    {
      TBucket* bucket = FindBucketWithKey(Key, false);
      if (!bucket) { return; }

      if (Head == bucket) { Head = bucket->Next; }
      if (Tail == bucket) { Tail = bucket->Previous; }

      if (bucket->Previous)
      {
        bucket->Previous->Next = bucket->Next;
      }

      if (bucket->Next)
      {
        bucket->Next->Previous = bucket->Previous;
      }

      NumBuckets++;
      Pair<K, V>& data = *bucket;
      data.~Pair();
      new (bucket) TBucket();
      bucket->Status = libenum::BucketStatus::Deleted;

      if (!Length())
      {
        Head = Tail = nullptr;
      }
    }

    V* GetValueForKey(const K& Key)
    {
      TBucket* bucket = FindBucketWithKey(Key);
      if ((bucket == nullptr) ||
        (bucket->Status == libenum::BucketStatus::Empty) ||
        (bucket->Status == libenum::BucketStatus::Deleted))
      {
        return nullptr;
      }
      return &bucket->Value;
    }

    template <typename KeyType, typename... ForwardType>
    Pair<K, V>& EmplaceInternal(KeyType&& Arg, ForwardType&&... Args)
    {
      width_t capacity = Entries.Size();
      if (!NumBuckets)
      {
        VKT_ASSERT(Container::IsDynamic::Value && "Cannot resize container with static width!");
        if (Container::IsDynamic::Value)
        {
          NumBuckets = Entries.GetSlack();
        }
        Entries.Reserve(capacity + NumBuckets);
        capacity = Entries.Size();
      }

      width_t constant = 0;
      width_t hash = Hasher()(Arg);
      width_t index = GetHashIndex(hash, constant++, capacity);

      if (Entries.Length() < index)
      {
        Entries.Resize(capacity - 1);
      }

      TBucket* bucket = &Entries[index];

      while (bucket->Status == libenum::BucketStatus::Occupied)
      {
        index = GetHashIndex(hash, constant++, capacity);
      }

      bucket = &Entries.EmplaceAt(index, Forward<KeyType>(Arg), Forward<ForwardType>(Args)...);
      bucket->Status = libenum::BucketStatus::Occupied;

      NumBuckets--;

      if (!Head && !Tail)
      {
        Head = Tail = bucket;
      }
      else
      {
        Tail->Next = bucket;
        bucket->Previous = Tail;
        Tail = bucket;
      }
      return *bucket;
    }

    template <typename KeyType>
    Pair<K, V>& TryEmplace(KeyType&& Key)
    {
      TBucket* pBucket = FindBucketWithKey(Key);
      if (!pBucket)
      {
        return EmplaceInternal(Forward<KeyType>(Key), V());
      }
      return *pBucket;
    }

  public:

    MapImpl() :
      BaseMap<K, V, UintWidth>{}, Entries{}
    {
      NumBuckets = Entries.Size();
    }
    ~MapImpl() { Release(); }
    MapImpl(const MapImpl& Rhs) { *this = Rhs; }
    MapImpl(MapImpl&& Rhs) { *this = Move(Rhs); }
    MapImpl& operator=(const MapImpl& Rhs)
    {
      if (this != &Rhs)
      {
        Entries = Rhs.Entries;
        BaseMap<K, V, UintWidth>::operator=(Rhs);
      }
      return *this;
    }
    MapImpl& operator=(MapImpl&& Rhs)
    {
      if (this != &Rhs)
      {
        Entries = Move(Rhs.Entries);
        BaseMap<K, V, UintWidth>::operator=(Move(Rhs));
      }
      return *this;
    }

    template <typename... ForwardType>
    Pair<K, V>& Emplace(ForwardType&&... Args)
    {
      return EmplaceInternal(Forward<ForwardType>(Args)...);
    }

    template <typename... ForwardType>
    V& Insert(ForwardType&&... Args)
    {
      Pair<K, V>& pair = Emplace(Forward<ForwardType>(Args)...);
      return pair.Value;
    }

    void Remove(const K& Key)
    {
      RemoveBucketWithKey(Key);
    }

    void Release()
    {
      Entries.Release();
    }

    void Empty()
    {
      Entries.Empty();
      NumBuckets = Entries.Size();
      Head = Tail = nullptr;
    }

    bool IsEmpty() const
    {
      return NumBuckets == Entries.Size();
    }

    width_t Length() const
    {
      return Entries.Size() - NumBuckets;
    }

    bool Contains(const K& Key)
    {
      if (!Entries.Size()) { return false; }
      return (FindBucketWithKey(Key) != nullptr);
    }

    V& operator[] (const K& Key)
    {
      return TryEmplace(Key).Value;
    }

    V& operator[] (K&& Key)
    {
      return TryEmplace(Move(Key)).Value;
    }

    const V& operator[] (const K& Key) const
    {
      V* value = GetValueForKey(Key);
      VKT_ASSERT(value && "Specified key does not exist in the container.");
      return *value;
    }

    Iterator                begin() { return Iterator(Head); }
    Iterator                end() { return Iterator(nullptr); }
    ConstIterator           begin() const { return ConstIterator(Head); }
    ConstIterator           end() const { return ConstIterator(nullptr); }
    ReverseIterator         rbegin() { return ReverseIterator(Tail); }
    ReverseIterator         rend() { return ReverseIterator(nullptr); }
    ConstReverseIterator    rbegin() const { return ConstReverseIterator(Tail); }
    ConstReverseIterator    rend() const { return ConstReverseIterator(nullptr); }
  };

  template <typename K, typename V, size_t Slack = 3, typename Hasher = XxHash<K>, typename UintWidth = size_t>
  using Map = MapImpl<K, V, Hasher, Array<typename BaseMap<K, V, UintWidth>::TBucket, UintWidth, Slack>, UintWidth>;

  template <typename K, typename V, size_t Capacity, typename Hasher = XxHash<K>, typename UintWidth = size_t>
  using StaticMap = MapImpl<K, V, Hasher, StaticArray<typename BaseMap<K, V, UintWidth>::TBucket, Capacity, UintWidth>, UintWidth>;

}

#endif // !ANGKASA1_HASH_MAP
