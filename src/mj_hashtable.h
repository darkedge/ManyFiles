#pragma once
#include "mj_allocator.h"
#include "mj_common.h"

namespace mj
{
  class HashTable
  {
  private:
    AllocatorBase* pAllocator            = nullptr;
    size_t numBuckets                    = 0;
    mj::ArrayList<const void*>* pBuckets = nullptr;
    friend class Iterator;

  public:
    void Init(AllocatorBase* pAllocator)
    {
      this->pAllocator = pAllocator;
      this->numBuckets = 10;

      this->pBuckets = this->pAllocator->New<mj::ArrayList<const void*>>(this->numBuckets);
      for (size_t i = 0; i < numBuckets; i++)
      {
        this->pBuckets[i].Init(pAllocator);
      }
    }

    void Destroy()
    {
      if (this->pAllocator)
      {
        for (size_t i = 0; i < numBuckets; i++)
        {
          this->pBuckets[i].Destroy();
        }
        this->pAllocator->Free(this->pBuckets);

        this->pBuckets   = nullptr;
        this->pAllocator = nullptr;
        this->numBuckets = 0;
      }
    }

    void Insert(const void* ptr)
    {
      size_t index = reinterpret_cast<uintptr_t>(ptr) % this->numBuckets;
      if (!this->pBuckets[index].Contains(ptr))
      {
        this->pBuckets[index].Add(ptr);
      }
    }

    void Remove(const void* ptr)
    {
      size_t index = reinterpret_cast<uintptr_t>(ptr) % this->numBuckets;
      this->pBuckets[index].RemoveAll(ptr);
    }

    class Iterator
    {
    private:
      const HashTable* pHashTable;
      size_t bucketIndex = 0;
      const void** pElement;

    public:
      void Init(const HashTable* pHashTable, size_t bucketIndex, const void** pElement)
      {
        this->pHashTable  = pHashTable;
        this->bucketIndex = bucketIndex;
        this->pElement    = pElement;
      }

      const Iterator& operator++()
      {
        this->pElement++;
        auto pBucket = pHashTable->pBuckets[this->bucketIndex];

        if (this->pElement == pBucket.end())
        {
          // Go to the next bucket. We do not need to check if the next bucket is valid,
          // because we only increment this iterator if we haven't reached the last element yet.
          do
          {
            this->bucketIndex++;
          } while (pHashTable->pBuckets[this->bucketIndex].Size() == 0);

          this->pElement = pHashTable->pBuckets[this->bucketIndex].begin();
        }

        return *this;
      }

      Iterator operator++(int)
      {
        Iterator result = *this;
        ++(*this);
        return result;
      }

      const void* operator*()
      {
        return *pElement;
      }

      bool operator!=(const Iterator& other)
      {
        return this->pElement != other.pElement;
      }
    };

    Iterator begin() const
    {
      Iterator iterator;
      iterator.Init(this, 0, nullptr);

      for (size_t i = 0; i < this->numBuckets; i++)
      {
        if (this->pBuckets[i].Size() > 0)
        {
          iterator.Init(this, i, this->pBuckets[i].begin());
          break;
        }
      }
      return iterator;
    }

    Iterator end() const
    {
      Iterator iterator;
      iterator.Init(this, 0, nullptr);

      for (size_t i = 0; i < this->numBuckets; i++)
      {
        size_t j = numBuckets - 1 - i;
        if (this->pBuckets[j].Size() > 0)
        {
          iterator.Init(this, j, this->pBuckets[j].end());
          break;
        }
      }

      return iterator;
    }
  };
} // namespace mj
