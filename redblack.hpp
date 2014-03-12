#include <iostream>
#include <exception>
#include <stdexcept>

using namespace std;

// Copyright Adrian McMenamin 2010 - 2014
// Licensed under the GNU GPL version 2
// or any later version at your discretion

#ifndef _RBTREECPP_
#define _RBTREECPP_

template <typename T>
class redblacknode{

	template <typename Z> friend ostream& operator<<(ostream& os,
		redblacknode<Z>* rbtp);
	template <typename Z> friend void 
		streamrbt(ostream& os, redblacknode<Z>* node);

	private:
		T value;
	
	public:
		int colour;
		redblacknode* up;
		redblacknode* left;
		redblacknode* right;
		redblacknode(T& v);
		redblacknode(redblacknode* node);
		redblacknode(redblacknode& node);
		redblacknode* grandparent() const;
 		redblacknode* uncle() const;
		redblacknode* sibling() const;
		T& getvalue();
		bool bothchildrenblack() const;
		bool equals(redblacknode*) const;
		bool lessthan(redblacknode*) const;
		void assign(redblacknode*);
		void showinorder(redblacknode*) const;
		void showpreorder(redblacknode*) const;
		void showpostorder(redblacknode*) const;
};

template <typename NODE>
class redblacktree {
	private:
		void balanceinsert(NODE*);
		void rotate3(NODE*);
		void rotate2(NODE*);
		void rotate2a(NODE*);
		void rotate1(NODE*);
		void transform2(NODE*);
		void free(NODE*);
		NODE* maxsuc(NODE*) const;
		NODE* minsuc(NODE*) const;
		int countup(NODE*) const;
		void leftrotate(NODE*);
		void rightrotate(NODE*);
		void deletefixup(NODE*);
	public:
		NODE* root;
		NODE* locatenode(NODE*, NODE*) const;  
		void insertnode(NODE*, NODE*);
		bool removenode(NODE&);
		bool find(NODE&) const;
		NODE* min() const;
		NODE* max() const;
		const int count() const;
		redblacktree();
		~redblacktree();
};

template <typename T> T& redblacknode<T>::getvalue()
{
	return value;
}

template <typename T> void redblacknode<T>::showinorder(redblacknode<T>* node)
									const
{
	if (node == NULL)
		return;
	showinorder(node->left);
	cout << node->value << ", ";
	showinorder(node->right);
}

template <typename T> void redblacknode<T>::showpreorder(redblacknode<T>* node)
									const
{
	if (node == NULL)
		return;
	cout << node->value << ", ";
	showpreorder(node->left);
	showpreorder(node->right);
}

template <typename T> void redblacknode<T>::showpostorder(redblacknode<T>* node)
									const
{
	if (node == NULL)
		return;
	showpostorder(node->left);
	showpostorder(node->right);
	cout << node->value << ", ";
}


template <typename T> redblacknode<T>::redblacknode(T& v): value(v)
{
	colour = 1; //red
	up = NULL;
	left = NULL;
	right = NULL;
}

template <typename T> redblacknode<T>::redblacknode(redblacknode* node)
{
	colour = node->colour;
	value = node->value;
	up = NULL;
	left = NULL;
	right = NULL;
}

template <typename T> redblacknode<T>::redblacknode(redblacknode& node)
{
	colour = node.colour;
	value = node.value;
	up = NULL;
	left = NULL;
	right = NULL;
}

template <typename T> redblacknode<T>* redblacknode<T>::sibling() const
{
	if (!up)
		return NULL;
	if (up->left == this)
		return up->right;
	else
		return up->left;
}

template <typename T> redblacknode<T>* redblacknode<T>::grandparent() const
{
	if (up)
		return up->up;
	else
		return NULL;
}

template <typename T> redblacknode<T>* redblacknode<T>::uncle() const
{
	redblacknode* g = grandparent();
	if (g) {
		if (g->left == up)
			return g->right;
		else
			return g->left;
	}
	return NULL;
}

template <typename T> bool redblacknode<T>::bothchildrenblack() const
{
	if (right && right->colour == 1)
		return false;
	if (left && left->colour == 1)
		return false;
	return true;
}


template <typename T> bool redblacknode<T>::equals(redblacknode* rbn) const
{
	if (value == rbn->value)
		return true;
	return false;
}

template <typename T> bool redblacknode<T>::lessthan(redblacknode* rbn) const
{
	if (value < rbn->value)
		return true;
	return false;
}

template <typename T> void redblacknode<T>::assign(redblacknode<T>* v)
{
	value = v->value;
}

template <typename NODE> redblacktree<NODE>::~redblacktree()
{
	free(root);
}

template <typename NODE> void redblacktree<NODE>::free(NODE* v)
{
	if (v == NULL)
		return;
	free(v->left);
	NODE* tmp = v->right;
	delete v;
	free(tmp);
}

template <typename NODE> redblacktree<NODE>::redblacktree()
{
	root = NULL;
}

// turn line of two reds and a black into black with two children
template <typename NODE> void redblacktree<NODE>::rotate2(NODE* node)
{
	if (!node || !node->up)
		return;
	NODE* gp = node->grandparent();
	NODE* par = NULL;
	
	NODE* centrenode = node->up;
	if (gp) {
		par = gp->up;
		if (par) {
			if (par->left == gp)
				par->left = centrenode;
			else
				par->right = centrenode;
		} 
	}
	
	if (node->up->right == node)
	{
		NODE* centreleft = centrenode->left;
		centrenode->colour = 0;
		centrenode->left = gp;
		if (gp) {
			gp->up = centrenode;
			gp->colour = 1;
			gp->right = centreleft;
			if (centreleft)
				centreleft->up = gp;
		}
	} else {
		NODE* centreright = centrenode->right;
		centrenode->colour = 0;
		centrenode->right = gp;
		if (gp) {
			gp->up = centrenode;
			gp->colour = 1;
			gp->left = centreright;
			if (centreright)
				centreright->up = gp;
		}
	}
	centrenode->up = par;
	if (!par)
		root = centrenode;
}

template <typename NODE> void redblacktree<NODE>::rotate3(NODE* node)
{
	if (!node || !node->up)
		return;
	NODE* par = node->up;
	NODE* righty = node->right;
	NODE* lefty = node->left;

	if (par->left == node) {
		par->left = righty;
		righty->colour = 0;
		righty->up = par;
		node->up = righty;
		node->right = righty->left;
		righty->left = node;
		node->colour = 1;
	}
	else {
		par->right = lefty;
		lefty->colour = 0;
		lefty->up = par;
		node->up = lefty;
		node->left = lefty->right;
		lefty->right = node;
		node->colour = 1;
	}
}			

template <typename NODE> void redblacktree<NODE>::rotate2a(NODE* node)
{
	if (!node || !node->up)
		return;
	NODE* par = node->up;
	NODE* gp = node->grandparent();
	NODE* righty = node->right;
	NODE* lefty = node->left;
	if (gp) {
		if (gp->left == par)
			gp->left = node;
		else
			gp->right = node;
		node->up = gp;
	} else {
		root = node;
		node->up = NULL;
	}

	if (par->right == node) {
		node->left = par;
		par->up = node;
		par->right = lefty;
		if (lefty)
			lefty->up = par;

	} else {
		node->right = par;
		par->up = node;
		par->left = righty;
		if (righty)
			righty->up = par;
	}
	par->colour = 1;
	node->colour = 0;
}	

//straighten zig zag of two reds
template <typename NODE> void redblacktree<NODE>::rotate1(NODE* node)
{
	if (!node)
		return;
	NODE* par = node->up;
	NODE* rightnode = node->right;
	NODE* leftnode = node->left;
	NODE* rightleft = NULL;
	NODE* leftright = NULL;

	if (par) {
		if (par->left == node) {
			par->left = rightnode;
			if (rightnode) {
				rightleft = rightnode->left;
				rightnode->up = par;
				rightnode->left = node;
			}
			node->right = rightleft;
			if (rightleft)
				rightleft->up = node;
			node->up = rightnode;
		} else {
			par->right = leftnode;
			if (leftnode) {
				leftright = leftnode->right;
				leftnode->up = par;
				leftnode->right = node;
			}
			node->left = leftright;
			if (leftright)
				leftright->up = node;
			node->up = leftnode;
		}
	}
}

template <typename NODE> void redblacktree<NODE>::transform2(NODE* node)
{
	int oldcolour = node->up->colour;
	rotate2a(node);
	node->colour = oldcolour;
	if (node->left)
		node->left->colour = 0;
	if (node->right)
		node->right->colour = 0;
}

template <typename NODE> void redblacktree<NODE>::balanceinsert(NODE* node)
{
	if (node->up) {
		if (node->up->colour == 0) {
			return;}

		if (node->uncle() && node->uncle()->colour == 1) {
			node->up->colour = 0;
			node->uncle()->colour = 0;
			node->grandparent()->colour = 1;
			balanceinsert(node->grandparent());
		} else {
			
			if (node->grandparent()->left == node->up) {
				if (node->up->right == node){ 
					rotate1(node->up);
					node = node->left;
				}
				rotate2(node);
			} else {
				if (node->up->left == node){
					rotate1(node->up);
					node = node->right;
				}
				rotate2(node);
			}
		}
		return;
	}
	else 
		node->colour = 0;

}

template <typename NODE> void redblacktree<NODE>::insertnode(NODE* insert,
							NODE* node)
{
	if (node == NULL) {
		root = insert;
		root->colour = 0;
		return;
	}
	if (insert->lessthan(node)) { 
		if (node->left == NULL) {
			node->left = insert;
			node->left->up = node;
			node = node->left;
			balanceinsert(node);
		} else 
			insertnode(insert, node->left);
	} else {
		if (node->right == NULL) {
			node->right = insert;
			node->right->up = node;
			node = node->right;
			balanceinsert(node);
		} else
			insertnode(insert, node->right);
	}
}

template <typename NODE> NODE* redblacktree<NODE>::locatenode(NODE* v,
		NODE* node) const
{
	if (node == NULL)
		return node;
	if (v->equals(node))
		return node;
	if (v->lessthan(node))
		return locatenode(v, node->left);
	else
		return locatenode(v, node->right);
}

template <typename NODE> NODE* redblacktree<NODE>::minsuc(NODE* node) const
{

	if (node->left)
		return minsuc(node->left);
	else
		return node;
}

template <typename NODE> NODE* redblacktree<NODE>::min() const
{
	if (!root)
		return NULL;
	NODE* p = root;
	do {
		if (p->left == NULL)
			return p;
		p = p->left;
	} while(true);
}

template <typename NODE> NODE* redblacktree<NODE>::max() const
{
	if (!root)
		return NULL;
	NODE* p = root;
	do {
		if (p->right == NULL)
			return p;
		p = p->right;
	} while(true);
}

template <typename NODE> int 
	redblacktree<NODE>::countup(NODE* node) const
{
	int count = 0;
	if (node != NULL) {
		count = 1;
		count += countup(node->left);
		count += countup(node->right);
	}
	return count;
}

template <typename NODE> const int redblacktree<NODE>::count() const
{
	return countup(root);
}

template <typename NODE> NODE* redblacktree<NODE>::maxsuc(NODE* node) const
{
	if (node->right)
		return maxsuc(node->right);
	else
		return node;
}

template <typename NODE> bool redblacktree<NODE>::find(NODE& v) const
{
	NODE* located = locatenode(&v, root);
	if (located)
		return true;
	else
		return false;
}

template <typename NODE> void redblacktree<NODE>::leftrotate(NODE* xnode)
{
	NODE* ynode = xnode->right;
	if (ynode) {
		NODE* tmp = new NODE(xnode->getvalue());
		tmp->assign(xnode);
		tmp->left = xnode->left;
		tmp->right = ynode->left;
		tmp->up = xnode;
		xnode->assign(ynode);
		xnode->left = tmp;
		xnode->right = ynode->right;
		delete ynode;
	}
}

template <typename NODE> void redblacktree<NODE>::rightrotate(NODE* xnode)
{
	NODE* ynode = xnode->up;
	if (ynode) {
		NODE* tmp = new NODE(ynode->getvalue());
		tmp->assign(ynode);
		tmp->left = xnode->right;
		tmp->right = ynode->right;
		ynode->assign(xnode);
		ynode->left = xnode->left;
		ynode->right = tmp;
		tmp->up = ynode;
		delete xnode;
	}
}

template <typename NODE> void redblacktree<NODE>::deletefixup(NODE* xnode)
{
	if (xnode == NULL) {
		return;
	}
	NODE* wnode = NULL;
	while (xnode && xnode != root && xnode->colour == 0) {
		if (xnode == xnode->up->left) {
			wnode = xnode->up->right;
			if (wnode && wnode->colour == 1) {
				wnode->colour = 0;
				xnode->up->colour = 1;
				leftrotate(xnode->up);
				wnode = xnode->up->right;
			}
			if ((!(wnode->left) || wnode->left->colour == 0) &&
				(!(wnode->right) || wnode->right->colour == 0))
			{
				wnode->colour = 1;
				xnode = xnode->up;
			} else {
				if (!(wnode->right) || wnode->right->colour==0)
				{
					if (wnode->left) {
						wnode->left->colour = 0;
					}
					wnode->colour = 1;
					rightrotate(wnode);
					wnode = xnode->up->right;
				}
				wnode->colour = xnode->up->colour;
				xnode->up->colour = 0;
				wnode->right->colour = 0;
				leftrotate(xnode->up);
				xnode = root;
			}
		} else {
			wnode = xnode->up->left;
			if (wnode && wnode->colour == 1) {
				wnode->colour = 0;
				xnode->up->colour = 1;
				rightrotate(xnode->up);
				wnode = xnode->up->left;
			}
			if ((!(wnode-> right) || wnode->right->colour == 0) &&
				(!(wnode->left) || wnode->left->colour == 0))
			{
				wnode->colour = 1;
				xnode = xnode->up;
			} else {
				if (!(wnode->left) || wnode->left->colour == 0)
				{
					if (wnode->right) {	
						wnode->right->colour = 0;
					}
					wnode->colour = 1;
					leftrotate(wnode);
					wnode = xnode->up->left;
				}
				wnode->colour = xnode->up->colour;
				xnode->up->colour = 0;
				wnode->left->colour = 0;
				rightrotate(xnode->up);
				xnode = root;
			}
		}
	}
}

template <typename NODE> bool redblacktree<NODE>::removenode(NODE& v)
{
	//basic checks then find node
	if (&v == NULL) {
		throw invalid_argument("Attempted to remove NULL node");
	}
	NODE* znode = locatenode(&v, root);
	if (!znode) {
		return false;
	}
	NODE* ynode = NULL;
	NODE* xnode = NULL;
	if (znode->left == NULL || znode->right == NULL) {
		ynode = znode;
	} else {
		if (znode->right) {
			ynode = minsuc(znode->right); //successor
		}
	}

	if (ynode->left) {
		xnode = ynode->left;
	} else {
		if (ynode->right) {
			xnode = ynode->right;
		}
	}
	
	if (xnode) {
		xnode->up = ynode->up;
	}

	if (ynode->up == NULL) {
		root = xnode;
	} else {
		if (ynode == ynode->up->left){
			ynode->up->left = xnode;
		} else {
			ynode->up->right = xnode;
		}
	}

	if (ynode != znode) {
		znode->assign(ynode);
	}

	if (!ynode || ynode->colour == 0) {
		deletefixup(xnode);
		//fixup code goes here
	}
	delete ynode;
	return true;
}

template <typename T> void streamrbt(ostream& os, redblacknode<T>* node)
{
	if (node == NULL) 
		return;
	os << "(" << node->value;
	if (node->colour == 0)
		os << "[BLACK]";
	else
		os << "[RED]";
	streamrbt(os, node->left);
	cout << ",";
	streamrbt(os, node->right);
	os << ")";
}

template <typename T> ostream& operator<<(ostream& os, redblacknode<T>* rbn)
{
	streamrbt(os, rbn);
	return os;
}		

#endif
