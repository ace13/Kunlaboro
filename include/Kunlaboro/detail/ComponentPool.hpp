#pragma once

#include <cstdint>
#include <vector>

namespace Kunlaboro
{

	namespace detail
	{
		
		class BaseComponentPool
		{
		public:
			BaseComponentPool(std::size_t componentSize, std::size_t chunkSize = 256);
			virtual ~BaseComponentPool();

			inline std::size_t getSize() const { return mSize; }
			inline std::size_t getComponentSize() const { return mComponentSize; }
			inline std::size_t getChunkSize() const { return mChunkSize; }

			void ensure(std::size_t count);
			void resize(std::size_t count);

			bool hasBit(std::size_t index) const;
			void setBit(std::size_t index);
			void resetBit(std::size_t index);

			inline void* getData(std::size_t index) {
				return mBlocks[index / mChunkSize] + (index % mChunkSize) * mComponentSize;
			}
			inline const void* getData(std::size_t index) const {
				return mBlocks[index / mChunkSize] + (index % mChunkSize) * mComponentSize;
			}
			virtual void destroy(std::size_t index) = 0;

		private:
			std::vector<uint8_t*> mBlocks;
			std::vector<uint64_t> mBits;
			std::size_t mComponentSize, mChunkSize, mSize, mCapacity;
		};


		template<typename T, std::size_t ChunkSize = 256>
		class ComponentPool : public BaseComponentPool
		{
		public:
			ComponentPool()
				: BaseComponentPool(sizeof(T), ChunkSize)
			{ }
			virtual ~ComponentPool() { }

			virtual void destroy(std::size_t index) override
			{
				static_cast<T*>(getData(index))->~T();
			}
		};

	}

}
