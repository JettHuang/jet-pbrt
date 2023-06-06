// bounding volume hierarchy
//
//

#pragma once

#include "pbrt.h"
#include "geometry.h"

namespace pbrt
{

	class FIntersection;

#define MAX_HITTABLES_IN_LEAF	5

	inline bool box_compare(const FBounds3& a, const FBounds3& b, eAxis axis)
	{
		return a._min[axis] < b._min[axis];
	}

	template<typename T>
	bool box_x_compare(const T& a, const T& b) {
		return box_compare(a->WorldBounds(), b->WorldBounds(), eAxis::AXIS_X);
	}

	template<typename T>
	bool box_y_compare(const T& a, const T& b) {
		return box_compare(a->WorldBounds(), b->WorldBounds(), eAxis::AXIS_Y);
	}

	template<typename T>
	bool box_z_compare(const T& a, const T& b) {
		return box_compare(a->WorldBounds(), b->WorldBounds(), eAxis::AXIS_Z);
	}

	// bvh node
	class FBVH_NodeBase
	{
	public:
		FBVH_NodeBase() {}
		virtual ~FBVH_NodeBase() {}

		virtual bool Intersect(const FRay& ray, FIntersection& oisect) const = 0;
		const FBounds3& bounding_box() const
		{
			return bbox;
		}

	protected:
		FBounds3 bbox;
	};

	template<typename T>
	class FBVH_Node : public FBVH_NodeBase
	{
	public:
		// [start, end)
		FBVH_Node(std::vector<T>& objects, size_t start, size_t end)
		{
			int axis = random_int(0, 2);
			auto comparator = (axis == eAxis::AXIS_X) ? box_x_compare<T>
				: (axis == eAxis::AXIS_Y) ? box_y_compare<T> : box_z_compare<T>;

			size_t object_span = end - start;

			if (object_span <= MAX_HITTABLES_IN_LEAF)
			{
				std::shared_ptr<FBVH_NodeLeaf<T>> leafNode = std::make_shared<FBVH_NodeLeaf<T>>(objects, start, end);
				left = leafNode;
				shadow_left = left.get();
				shadow_right = nullptr;
			}
			else
			{
				std::sort(objects.begin() + start, objects.begin() + end, comparator);

				auto mid = start + object_span / 2;
				left = std::make_shared<FBVH_Node<T>>(objects, start, mid);
				right = std::make_shared<FBVH_Node<T>>(objects, mid, end);

				shadow_left = left.get();
				shadow_right = right.get();
			}

			FBounds3 box_left, box_right;
			
			if (left) { box_left = left->bounding_box(); }
			if (right) { box_right = right->bounding_box(); }

			bbox = box_left.Join(box_right);
		}

		virtual bool Intersect(const FRay& ray, FIntersection& oisect) const
		{
			if (!bbox.Intersect(ray))
				return false;

			bool hit_left = shadow_left->Intersect(ray, oisect);
			bool hit_right = shadow_right ? shadow_right->Intersect(ray, oisect) : false;

			return hit_left || hit_right;
		}

	protected:
		std::shared_ptr<FBVH_NodeBase> left;
		std::shared_ptr<FBVH_NodeBase> right;

		FBVH_NodeBase* shadow_left;
		FBVH_NodeBase* shadow_right;
	};


	template<typename T>
	class FBVH_NodeLeaf : public FBVH_NodeBase
	{
	public:
		// [start, end)
		FBVH_NodeLeaf(std::vector<T>& objects, size_t start, size_t end)
		{
			FBounds3 boundingbox;
			for (size_t i = start; i < end; i++)
			{
				objs.push_back(objects[i]);

				boundingbox.Expand(objects[i]->WorldBounds());
			}

			bbox = boundingbox;
		}

		virtual bool Intersect(const FRay& ray, FIntersection& oisect) const
		{
			bool bHit = false;

			for (auto obj : objs)
			{
				bHit |= obj->Intersect(ray, oisect);
			}

			return bHit;
		}

	protected:
		std::vector<T> objs;
	};

} // namespace pbrt

