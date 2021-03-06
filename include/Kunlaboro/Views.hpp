#pragma once

#include "ID.hpp"
#include "Entity.hpp"

#include "detail/Delegate.hpp"
#include "detail/DynamicBitfield.hpp"

#include <functional>
#include <type_traits>

namespace Kunlaboro
{
	namespace detail { class JobQueue; }

	class Component;
	class EntitySystem;

	enum MatchType
	{
		Match_All,
		Match_Any
	};

	namespace impl
	{

		template<typename ViewType, typename ViewedType>
		class BaseView
		{
		public:
			typedef detail::Delegate<bool(const ViewedType&)> Predicate;

			virtual ~BaseView();

			/** Enables or disables parallel view iteration.
			 *
			 * \param parallell Should the view be iterated in parallel or sequentially.
			 */
			ViewType parallel(bool parallell = true) const;
			/** Sets the view to iterate in parallel, using the given job queue.
			 *
			 * \param queue The job queue to use for the iteration.
			 */
			ViewType parallel(detail::JobQueue& queue) const;

			/** Limits the view to values matching the given predicate function.
			 *
			 * \param pred The predicate to match.
			 */
			ViewType where(const Predicate& pred) const;

			const EntitySystem& getEntitySystem() const;
			const Predicate& getPredicate() const;
			void setPredicate(const Predicate& pred);

		protected:
			BaseView(const EntitySystem*);

			const EntitySystem* mES;
			Predicate mPred;

			detail::JobQueue* mQueue;
			bool mParallelOwned;
		};

		template<typename IteratorType, typename ViewedType>
		class BaseIterator : public std::iterator<std::input_iterator_tag, ViewedType>
		{
		public:
			typedef detail::Delegate<bool(const ViewedType&)> Predicate;
			virtual ~BaseIterator() = default;

			IteratorType& operator++();
			bool operator==(const BaseIterator& rhs) const;
			bool operator!=(const BaseIterator& rhs) const;

			virtual ViewedType* operator->() = 0;
			virtual const ViewedType* operator->() const = 0;
			virtual ViewedType& operator*() = 0;
			virtual const ViewedType& operator*() const = 0;

		protected:
			BaseIterator(const EntitySystem* es, uint64_t index, const Predicate& pred);

			void nextStep();
			virtual bool basePred() const { return true; }
			virtual void moveNext() = 0;
			virtual uint64_t maxLength() const = 0;

			const Predicate& mPred;
			const EntitySystem* mES;
			uint64_t mIndex;

		};

		bool matchBitfield(const detail::DynamicBitfield& entity, const detail::DynamicBitfield& bitField, MatchType match);
	}

	/** A view for iterating components in an entity system.
	 *
	 */
	template<typename T>
	class ComponentView : public impl::BaseView<ComponentView<T>, T>
	{
	public:
		static_assert(std::is_base_of<Component, T>::value, "Component Views only work on proper components.");

		ComponentView(const EntitySystem& es);

		/// The predicate function type.
		typedef detail::Delegate<bool(const T&)> Predicate;
		/// The function-call function type.
		typedef detail::Delegate<void(T&)> Function;

		struct Iterator : public impl::BaseIterator<Iterator, T>
		{
			inline T* operator->() { return mCurComponent.get(); }
			inline const T* operator->() const { return mCurComponent.get(); }
			inline T& operator*() { return *mCurComponent; }
			inline const T& operator*() const { return *mCurComponent; }

		protected:
			Iterator(const EntitySystem* sys, ComponentId componentBase, const Predicate& pred);

			friend class ComponentView;

			virtual bool basePred() const;
			virtual void moveNext();
			uint64_t maxLength() const;

		private:
			ComponentHandle<T> mCurComponent;
			const void* mComponents;
		};

		Iterator begin();
		Iterator end();

		virtual void forEach(const Function& func);
	};

	template<MatchType MT, typename... Components>
	class TypedEntityView;

	/** A view for iterating entities in the given entity system.
	 */
	class EntityView : public impl::BaseView<EntityView, Entity>
	{
	public:
		EntityView(const EntitySystem& es);

		typedef detail::Delegate<bool(const Entity&)> Predicate;
		typedef detail::Delegate<void(const Entity&)> Function;

		struct Iterator : public impl::BaseIterator<Iterator, Entity>
		{
			inline Entity* operator->() { return &mCurEntity; }
			inline const Entity* operator->() const { return &mCurEntity; }
			inline Entity& operator*() { return mCurEntity; }
			inline const Entity& operator*() const { return mCurEntity; }

		protected:
			Iterator(const EntitySystem* sys, EntityId::IndexType index, const Predicate& pred);

			friend class EntityView;

			virtual bool basePred() const;
			virtual void moveNext();
			uint64_t maxLength() const;

		private:
			Entity mCurEntity;
		};

		Iterator begin();
		Iterator end();

		/** Limits the entities to those with matching components.
		 *
		 * \tparam match The method for matching the components, can be either \p Match_Any or \p Match_All
		 * \tparam Components The components to match on the entities.
		 */
		template<MatchType match = Match_All, typename... Components>
		TypedEntityView<match, Components...> withComponents() const;

		virtual void forEach(const Function& func);
	};

	/** A view for iterating entities with given components in the given entity system.
	 *
	 * \tparam MT The method for matching the given components, can be either \p Match_Any or \p Match_All
	 * \tparam Components The component contained in the iterated entities.
	 */
	template<MatchType MT, typename... Components>
	class TypedEntityView : public impl::BaseView<TypedEntityView<MT, Components...>, Entity>
	{
		template<typename T> struct ident { typedef T type; };

	public:
		TypedEntityView(const EntitySystem& es);

		typedef std::function<bool(const Entity&)> Predicate;
		typedef std::function<void(Entity&)> Function;

		typedef EntityView::Iterator Iterator;

		Iterator begin();
		Iterator end();

		/** Iterates all entities, using either \p Match_Any or \p Match_All matching.
		 *
		 * \param func The function to call with every matching entity and its component pointers.
		 * \todo Make these work with the new delegates
		 */
		void forEach(const std::function<void(const Entity&, Components*...)>& func);
		/** Iterates all entities, using \p Match_All matching.
		 *
		 * \param func The function to call with every matching entity and its component references.
		 * \todo Make these work with the new delegates
		 */
		void forEach(const std::function<void(const Entity&, Components&...)>& func);

		virtual void forEach(const Function& func);

	private:
		template<typename T, typename T2, typename... ComponentsToAdd>
		inline void addComponents();
		template<typename T>
		inline void addComponents();

		detail::DynamicBitfield mBitField;
	};
}
