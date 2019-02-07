#ifndef AVITEMCONTAINER_H
#define AVITEMCONTAINER_H

template<class ItemType, class DeleterType, class UnreferencerType, class DoSomethingType>
class AVItemContainer {
	public:
		AVItemContainer() {
			item = nullptr;
			deleteItem = nullptr;
			unreferenceItem = nullptr;
			doSomethingWithItem = nullptr;
		}
		AVItemContainer(const AVItemContainer&) = delete;
		AVItemContainer& operator = (const AVItemContainer&) = delete;
		AVItemContainer(AVItemContainer&& other) {
			item = other.item;
			deleteItem = other.deleteItem;
			unreferenceItem = other.unreferenceItem;
			itemReferenced = other.itemReferenced;
			doSomethingWithItem = other.doSomethingWithItem;
			other.item = nullptr;
			other.deleteItem = nullptr;
			other.unreferenceItem = nullptr;
			other.itemReferenced = false;
			other.doSomethingWithItem = nullptr;
		}
		AVItemContainer& operator = (AVItemContainer&& other) {
			if(this == &other) return *this;
			resetPtr();
			item = other.item;
			deleteItem = other.deleteItem;
			unreferenceItem = other.unreferenceItem;
			itemReferenced = other.itemReferenced;
			doSomethingWithItem = other.doSomethingWithItem;
			other.item = nullptr;
			other.deleteItem = nullptr;
			other.unreferenceItem = nullptr;
			other.itemReferenced = false;
			other.doSomethingWithItem = nullptr;
		}

		~AVItemContainer() {
			if(item != nullptr) {
				if(itemReferenced) unreferenceItem(item);
				deleteItem(item);
			}
		}
		void unrefPtr() {
			if(item != nullptr) {
				if(itemReferenced) {
					unreferenceItem(item);
					itemReferenced = false;
				}
			}
		}
		void resetPtr() {
			if(item != nullptr) {
				if(itemReferenced) {
					unreferenceItem(item);
					itemReferenced = false;
				}
				deleteItem(item);
				item = nullptr;
			}
		}
		bool markPtrHowReferenced() {
			if(item == nullptr) return false;
			itemReferenced = true;
			return true;
		}
		bool markPtrHowUnreferenced() {
			if(item == nullptr) return false;
			itemReferenced = false;
			return true;
		}
		void doSomething() {
			if(item != nullptr && doSomethingWithItem != nullptr) {
				doSomethingWithItem(item);
			}
		}
		void setDeleter(DeleterType deleter) {
			deleteItem = deleter;
		}
		void setUnreferencer(UnreferencerType unreferencer) {
			unreferenceItem = unreferencer;
		}
		void setDoSomething(DoSomethingType doSomethingfnc) {
			doSomethingWithItem = doSomethingfnc;
		}
		bool hasDeleter() {
			return deleteItem != nullptr;
		}
		bool hasUnreferencer() {
			return unreferenceItem != nullptr;
		}
		bool hasDoSomething() {
			return doSomethingWithItem != nullptr;
		}
		ItemType* getPtr() {
			return item;
		}
		bool isReferenced() {
			return (item != nullptr) && itemReferenced;
		}
		void setUnreferencedPtr(ItemType* newItem) {
			if(item == newItem) return;
			if(item != nullptr) {
				if(itemReferenced) {
					unreferenceItem(item);
					itemReferenced = false;
				}
				deleteItem(item);
			}
			item = newItem;
		}
		void setReferencedPtr(ItemType* newItem) {
			setUnreferencedPtr(newItem);
			itemReferenced = true;
		}
	private:
		ItemType* item = nullptr;
		DeleterType deleteItem = nullptr;
		UnreferencerType unreferenceItem = nullptr;
		DoSomethingType doSomethingWithItem = nullptr;

		bool itemReferenced = false;
};

#endif // AVITEMCONTAINER_H
