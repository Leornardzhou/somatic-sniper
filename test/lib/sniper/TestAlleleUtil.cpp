#include "sniper/allele_util.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <utility>

using namespace std;

namespace {
    enum Allele {
        A = 1,
        C = 2,
        G = 4,
        T = 8
    };

    template<typename T1, typename T2>
    pair<int, int> mkpair(T1 a, T2 b) {
        return make_pair(int(a), int(b));
    }
}

TEST(AlleleUtil, count_alleles) {
    ASSERT_EQ(0, count_alleles(0));
    ASSERT_EQ(1, count_alleles(A));
    ASSERT_EQ(1, count_alleles(C));
    ASSERT_EQ(2, count_alleles(A|C));
    ASSERT_EQ(1, count_alleles(G));
    ASSERT_EQ(2, count_alleles(A|G));
    ASSERT_EQ(2, count_alleles(C|G));
    ASSERT_EQ(3, count_alleles(A|C|G));
    ASSERT_EQ(1, count_alleles(T));
    ASSERT_EQ(2, count_alleles(A|T));
    ASSERT_EQ(2, count_alleles(C|T));
    ASSERT_EQ(3, count_alleles(A|C|T));
    ASSERT_EQ(2, count_alleles(G|T));
    ASSERT_EQ(3, count_alleles(A|G|T));
    ASSERT_EQ(3, count_alleles(A|C|T));
    ASSERT_EQ(4, count_alleles(A|C|T|G));
}

TEST(AlleleUtil, genotype_set_difference) {
    EXPECT_EQ(A, genotype_set_difference(A|C, C));
    EXPECT_EQ(A|G, genotype_set_difference(A|C|G, C));
    EXPECT_EQ(0, genotype_set_difference(A|C, A|C));
    EXPECT_EQ(0, genotype_set_difference(A, A|C));
}

TEST(AlleleUtil, is_loh) {
    // 1, 2, 4, and 8 are single alleles, so LOH can't really happen
    for (int i = 0; i < 4; ++i) {
        int value = 1 << i;
        for (int j = 1; j <= 8; ++j)
            ASSERT_EQ(0, is_loh(j, value));
    }

    // these are all the possible ways that LOH can happen with 2/3 alleles
    // (we do not concern ourselves with N until later)
    vector< pair<int,int> > loh_pairs;
    loh_pairs.push_back(mkpair(A, A|C));
    loh_pairs.push_back(mkpair(C, A|C));

    loh_pairs.push_back(mkpair(A, A|G));
    loh_pairs.push_back(mkpair(G, A|G));

    loh_pairs.push_back(mkpair(A, A|T));
    loh_pairs.push_back(mkpair(T, A|T));

    loh_pairs.push_back(mkpair(C, C|G));
    loh_pairs.push_back(mkpair(G, C|G));

    loh_pairs.push_back(mkpair(C, C|T));
    loh_pairs.push_back(mkpair(T, C|T));

    loh_pairs.push_back(mkpair(G, G|T));
    loh_pairs.push_back(mkpair(T, G|T));

    loh_pairs.push_back(mkpair(A, A|C|G));
    loh_pairs.push_back(mkpair(C, A|C|G));
    loh_pairs.push_back(mkpair(G, A|C|G));
    loh_pairs.push_back(mkpair(A|C, A|C|G));
    loh_pairs.push_back(mkpair(A|G, A|C|G));
    loh_pairs.push_back(mkpair(C|G, A|C|G));

    loh_pairs.push_back(mkpair(A, A|C|T));
    loh_pairs.push_back(mkpair(C, A|C|T));
    loh_pairs.push_back(mkpair(T, A|C|T));
    loh_pairs.push_back(mkpair(A|C, A|C|T));
    loh_pairs.push_back(mkpair(A|T, A|C|T));
    loh_pairs.push_back(mkpair(C|T, A|C|T));

    loh_pairs.push_back(mkpair(A, A|G|T));
    loh_pairs.push_back(mkpair(G, A|G|T));
    loh_pairs.push_back(mkpair(T, A|G|T));
    loh_pairs.push_back(mkpair(A|G, A|G|T));
    loh_pairs.push_back(mkpair(A|T, A|G|T));
    loh_pairs.push_back(mkpair(G|T, A|G|T));

    loh_pairs.push_back(mkpair(C, C|G|T));
    loh_pairs.push_back(mkpair(G, C|G|T));
    loh_pairs.push_back(mkpair(T, C|G|T));
    loh_pairs.push_back(mkpair(C|G, C|G|T));
    loh_pairs.push_back(mkpair(C|T, C|G|T));
    loh_pairs.push_back(mkpair(G|T, C|G|T));

    for (int orig = 1; orig < 15; ++orig) {
        for (int mut = 1; mut < 15; ++mut) {
            pair<int,int> p = make_pair(mut, orig);
            int expected = find(loh_pairs.begin(), loh_pairs.end(), p) != loh_pairs.end();
            ASSERT_EQ(expected, is_loh(int(p.first), int(p.second)));
        }
    }

    // deal with N here
    for (int i = 1; i < 15; ++i)
        ASSERT_EQ(1, is_loh(i, A|C|G|T));
}

TEST(AlleleUtil, should_filter_as_loh) {
    // these are all the possible ways that LOH can happen with 2/3 alleles
    // (we do not concern ourselves with N until later)
    int ref_base = A;

    ASSERT_TRUE(should_filter_as_loh(ref_base, A, A|G));
    ASSERT_TRUE(should_filter_as_loh(ref_base, G, A|G));
    ASSERT_TRUE(should_filter_as_loh(ref_base, G, C|G));
    ASSERT_TRUE(should_filter_as_loh(ref_base, C, C|G));
    ASSERT_FALSE(is_loh(A|G, G));
    ASSERT_TRUE(is_loh(G, A|G));
    ASSERT_TRUE(G != ref_base);
    // Tumor picks up the reference allele at a hom snp site in the normal.
    ASSERT_FALSE(should_filter_as_loh(ref_base, A|G, G));

    // Tests that hold across all tumor genotypes
    for (int i = 1; i < 15; ++i) {
        // With a hom ref normal, nothing should ever be filtered (as loh)
        ASSERT_FALSE(should_filter_as_loh(A, i, A))
            << "Alleles are: " << i;

        // Identical genotypes should never be filtered as LOH
        ASSERT_FALSE(should_filter_as_loh(A, i, i))
            << "Alleles are: " << i;
    }

    // With a het snp normal, picking up a new allele should not be filtered
    ASSERT_FALSE(should_filter_as_loh(A, A|C|G, A|C));
    ASSERT_FALSE(should_filter_as_loh(A, A|T, A|C));
    ASSERT_FALSE(should_filter_as_loh(A, T, A|C));

    // Same as above, picking up a new /non-ref/ allele in the tumor should
    // not be filtered
    ASSERT_FALSE(should_filter_as_loh(A, T|G, G));
    ASSERT_FALSE(should_filter_as_loh(A, C|G, G));
    ASSERT_FALSE (should_filter_as_loh(A, A|G, G)); // picked up ref, this is GOR

    // Going back to hom ref (T) from hom snp (N) is not filtered.
    ASSERT_FALSE(should_filter_as_loh(A, A, G));
}

TEST(AlleleUtil, should_filter_as_gor) {
    int ref_base = A;

    ASSERT_TRUE(should_filter_as_gor(ref_base, A, G));
    ASSERT_TRUE(should_filter_as_gor(ref_base, A|G, G));
    ASSERT_TRUE(should_filter_as_gor(ref_base, A|G, C|G));
    ASSERT_TRUE(should_filter_as_gor(ref_base, T|A, T|G));
    //
    // Going back to hom ref (T) from hom snp (N) is filtered.
    ASSERT_TRUE(should_filter_as_gor(A, A, G));

    // Tests that hold across all tumor genotypes
    for (int i = 1; i < 15; ++i) {
        // With a hom ref normal, nothing should ever be filtered (as gor)
        ASSERT_FALSE(should_filter_as_gor(A, i, A))
            << "Alleles are: " << i;

        // Identical genotypes should never be filtered as GOR
        ASSERT_FALSE(should_filter_as_gor(A, i, i))
            << "Alleles are: " << i;
    }

    // With a het snp normal, picking up a new, non-reference allele should not be filtered
    ASSERT_FALSE(should_filter_as_gor(A, A|C|G, A|C));
    ASSERT_FALSE(should_filter_as_gor(A, A|T, A|C));
    ASSERT_FALSE(should_filter_as_gor(A, T, A|C));
     
    // With a het snp normal, picking up a new, reference allele will be filtered
    // I'm not sure that this is relevant as Sniper doesn't handle triallelic genotypes in a sample
    // If it did handle it, I'm not sure this is the correct behaviour
    ASSERT_TRUE(should_filter_as_gor(A, A|T|C, T|C));

    // Same as above, picking up a new /non-ref/ allele in the tumor should
    // not be filtered
    ASSERT_FALSE(should_filter_as_gor(A, T|G, G));
    ASSERT_FALSE(should_filter_as_gor(A, C|G, G));
}
