n = poly(k)
hash h(x,y) = Lx + Ry mod q

label is a vector in [n]^klogq
label is an n-admissible raxid-2 representation of its digest
digest is a vector in Z_q^k


V1: From label of a node, one can compute its digest
V2: Given labels of the two children nodes, one can compute the digest of the parent node
P1: The label of a ndoe is the (weighted) sum of independent partial labels of the leaves in its subtree

Lemma 2
	Given the binary representations of x1 and x2, namely b1 and b2, a radix-2 representation of x1+x2 is b1+b2.

Definition 9: n-admissible radix-2 representation
	x is a vector Z_q^k.  n-admissible radix-2 representation r(x) \in Z_q^klogq if and only if r(x) \in {0, 1, .., n}^klogq
