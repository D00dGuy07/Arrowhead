#pragma once

#include <cstdint>
#include <cstdlib>
#include <utility>
#include <memory>

namespace arwh
{
	class Arena
	{
	public:
		Arena() = delete;
		~Arena() = delete;

		// Most basic functional implementation

		void* Push(size_t size);
		void Pop(size_t size);

		// Pushing helpers

		void* PushZero(size_t size);

		template<typename T>
		T* PushArray(uint64_t count)
		{
			return reinterpret_cast<T*>(Push(sizeof(T) * count));
		}

		template<typename T>
		T* PushArrayZero(size_t count)
		{
			return reinterpret_cast<T*>(PushZero(sizeof(T) * count));
		}

		template<typename T>
		T* PushStruct()
		{
			return reinterpret_cast<T*>(Push(sizeof(T)));
		}

		template<typename T>
		T* PushStructZero()
		{
			return reinterpret_cast<T*>(PushZero(sizeof(T)));
		}

		// Popping helpers

		size_t GetPos() const { return m_AllocatedSize; };
		void SetPosBack(size_t pos);
		void Clear();

		static Arena* Create(size_t size);
		static Arena* Create(void* buffer, size_t bufferSize);
		inline static void Dispose(Arena* arena) { if (arena->m_IsSelfAllocated) free(arena); }

		static void InitScratch();
		inline static void DisposeScratch() { Dispose(s_TempScratch); Dispose(s_PersistentScratch); }

		static Arena* GetTempScratch() { return s_TempScratch; }
		static Arena* GetPersistentScratch() { return s_PersistentScratch; }

	private:
		Arena(size_t size, bool selfAllocated);

		uint8_t* m_Data;
		uint8_t* m_Position;
		size_t m_TotalSize;
		size_t m_AllocatedSize = 0;
		bool m_IsSelfAllocated;

		inline static thread_local Arena* s_TempScratch = nullptr;
		inline static thread_local Arena* s_PersistentScratch = nullptr;
	};

	template<typename T>
	class PoolArenaAllocator
	{
	public:
		struct Node
		{
			T Value;
			Node* Next;
		};

		template<typename... Args>
		T* Allocate(Arena* arena, Args&&... args)
		{
			m_AllocationsCount++;

			Node* node = m_FirstFree;
			if (node != nullptr)
			{
				// Reuse one of the free nodes if possible
				m_FirstFree = node->Next;
				node->Next = nullptr;
			}
			else // Otherwise allocate a new one from the specified arena
				node = arena->PushStructZero<Node>();

			// Initialize the stored structure and return it
			new(&(node->Value)) T(std::forward<args>(args)...);
			return &node->Value;
		}

		void Free(T* value)
		{
			m_AllocationsCount--;

			// Cast the pointer up to a node and add it to the beginning of the free list
			Node* node = reinterpret_cast<Node*>(value);
			node->Next = m_FirstFree;
			m_FirstFree = node;
		}

		size_t GetAllocationsCount() const { return m_AllocationsCount; }

	private:
		Node* m_FirstFree = nullptr;
		size_t m_AllocationsCount = 0;
	};

	class ScratchSpace
	{
	public:
		ScratchSpace(Arena* arena);

		void Reset();

		inline Arena* GetArena() { return m_Arena; }
		inline bool HasReset() const { return m_HasReset; }
	private:
		Arena* m_Arena;
		size_t m_ResetPos;
		bool m_HasReset;
	};

	class StringBuilder
	{
	public:
		StringBuilder(Arena* arena, size_t size);

		StringBuilder& Push(const char* value);
		StringBuilder& operator<<(const char* value) { return Push(value); }
		char* Result() const { return m_Result; }



	private:
		char* m_Result;
		size_t m_Position = 0;
		size_t m_Size;
	};
}
