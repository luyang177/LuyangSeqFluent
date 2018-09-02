#ifndef LUYANGSEQ_H
#define LUYANGSEQ_H

#include <vector>
#include <functional>
#include <string>
#include <memory>

namespace LuyangSeq
{
	template<typename T>
	class Iterator
	{
	public:
		virtual ~Iterator() {}

		virtual bool HasNext() = 0;
		virtual T Next() = 0;
	};

	template<typename T>
	using IteratorPtr = std::shared_ptr<Iterator<T>>;

	template<typename T>
	class IteratorSame : public Iterator<T>
	{
	public:
		void SetInnerIterator(IteratorPtr<T> iter)
		{
			m_InnerIterator = iter;
		}
	protected:
		IteratorPtr<T> m_InnerIterator;
	};

	template<typename T>
	using IteratorSamePtr = std::shared_ptr<IteratorSame<T>>;

	template<typename TSource, typename TResult>
	class IteratorDiff : public Iterator<TResult>
	{
	public:
		void SetInnerIterator(IteratorPtr<TSource> iter)
		{
			m_InnerIterator = iter;
		}
	protected:
		IteratorPtr<TSource> m_InnerIterator;
	};

	template<typename TSource, typename TResult>
	using IteratorDiffPtr = std::shared_ptr<IteratorDiff<TSource, TResult>>;

	template<typename T>
	using Predicate = std::function<bool(T)>;

	template<typename T>
	class EmptyIterator : public Iterator<T>
	{
		bool HasNext() override
		{
			return false;
		}

		T Next() override
		{
			throw std::runtime_error("Wrong! Please call HasNext first");
		}
	};

	template<typename TContainer>
	class ContainerIterator : public Iterator<typename TContainer::value_type>
	{
	private:
		using ContainerIter = typename TContainer::iterator;
		using Value = typename TContainer::value_type;
	public:
		ContainerIterator(const TContainer& container)
			:m_Container(container),
			m_ContainerIter(m_Container.begin())
		{

		}

		bool HasNext() override
		{
			return m_ContainerIter != m_Container.end();
		}

		Value Next() override
		{
			return *m_ContainerIter++;
		}

	private:
		TContainer m_Container;
		ContainerIter m_ContainerIter;
	};

	template<typename T>
	class FilterIterator : public IteratorSame<T>
	{
	public:
		FilterIterator(Predicate<T> predicate)
			:m_Predicate(predicate)
		{

		}

		bool HasNext() override
		{
			while (this->m_InnerIterator->HasNext())
			{
				auto composeNext = this->m_InnerIterator->Next();
				if (m_Predicate(composeNext))
				{
					m_Current = composeNext;
					return true;
				}
			}

			return false;
		}

		T Next() override
		{
			return m_Current;
		}

	private:
		Predicate<T> m_Predicate;
		T m_Current;
	};

	template<typename T>
	class TakeIterator : public IteratorSame<T>
	{
	public:
		TakeIterator(int count)
			:m_Count(count),
			m_CurrentPos(0)
		{

		}

		bool HasNext() override
		{
			while (this->m_InnerIterator->HasNext())
			{
				if (m_CurrentPos == m_Count)
				{
					return false;
				}
				m_CurrentPos++;

				m_Current = this->m_InnerIterator->Next();
				return true;
			}

			return false;
		}

		T Next() override
		{
			return m_Current;
		}

	private:
		int m_Count;
		int m_CurrentPos;
		T m_Current;
	};

	template<typename TSource, typename TResult>
	using MapType = std::function<TResult(TSource)>;

	template<typename TSource, typename TResult>
	class MapIterator : public IteratorDiff<TSource, TResult>
	{
	private:
		using MapTypeName = MapType<TSource, TResult>;
	public:
		MapIterator(MapTypeName map)
			:m_Map(map)
		{

		}

		bool HasNext() override
		{
			while (this->m_InnerIterator->HasNext())
			{
				auto innerNext = this->m_InnerIterator->Next();
				m_Current = m_Map(innerNext);
				return true;
			}

			return false;
		}

		TResult Next()
		{
			return m_Current;
		}

	private:
		MapTypeName m_Map;
		TResult m_Current;
	};

	template<typename TSource, typename TResult>
	using FlatMapType = std::function<IteratorPtr<TResult>(TSource)>;

	template<typename TSource, typename TResult>
	class FlatMapIterator : public IteratorDiff<TSource, TResult>
	{
	private:
		using FlatMapTypeName = FlatMapType<TSource, TResult>;
	public:
		FlatMapIterator(FlatMapTypeName flatMap)
			:m_FlatMap(flatMap),
			m_FlatMapIterator(std::make_shared<EmptyIterator<TResult>>())
		{

		}

	public:

		bool HasNext() override
		{
			if (YieldFlatMapIter())
			{
				return true;
			}

			while (this->m_InnerIterator->HasNext())
			{
				auto innerNext = this->m_InnerIterator->Next();
				m_FlatMapIterator = m_FlatMap(innerNext);

				if (YieldFlatMapIter())
				{
					return true;
				}
			}

			return false;
		}

		TResult Next()
		{
			return m_Current;
		}

	private:
		bool YieldFlatMapIter()
		{
			while (m_FlatMapIterator->HasNext())
			{
				m_Current = m_FlatMapIterator->Next();
				return true;
			}

			return false;
		}

	private:
		FlatMapTypeName m_FlatMap;
		TResult m_Current;
		IteratorPtr<TResult> m_FlatMapIterator;
	};


	template<typename T>
	IteratorPtr<T> operator>>(IteratorPtr<T> iterA, IteratorSamePtr<T> iterB)
	{
		iterB->SetInnerIterator(iterA);
		return iterB;
	}

	template<typename A, typename B>
	IteratorPtr<B> operator>>(IteratorPtr<A> iterA, IteratorDiffPtr<A, B> iterB)
	{
		iterB->SetInnerIterator(iterA);
		return iterB;
	}

	#define Seq(container) (Seq::Apply(container))
	class Seq
	{
	public:
		template<typename TContainer>
		static IteratorPtr<typename TContainer::value_type> Apply(const TContainer& container)
		{
			return std::make_shared<ContainerIterator<TContainer>>(container);
		}

		template<typename T>
		static IteratorSamePtr<T> Filter(Predicate<T> predicate)
		{
			return std::make_shared<FilterIterator<T>>(predicate);
		}

		template<typename T>
		static IteratorSamePtr<T> Take(int count)
		{
			return std::make_shared<TakeIterator<T>>(count);
		}

		template<typename TSource, typename TResult>
		static IteratorDiffPtr<TSource, TResult> Map(MapType<TSource, TResult> map)
		{
			return std::make_shared<MapIterator<TSource, TResult>>(map);
		}

		template<typename TSource, typename TResult>
		static IteratorDiffPtr<TSource, TResult> FlatMap(FlatMapType<TSource, TResult> flatMap)
		{
			return std::make_shared<FlatMapIterator<TSource, TResult>>(flatMap);
		}
	};

}
#endif 