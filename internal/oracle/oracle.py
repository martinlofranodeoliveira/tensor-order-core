#!/usr/bin/env python3
"""Independent reference oracle for tensor contraction ordering and cost validation."""
import itertools
import json
import sys

def compute_boundary(tensors, subset_mask):
    """Compute boundary labels for a subset mask."""
    counts = {}
    for i, t in enumerate(tensors):
        if (subset_mask & (1 << i)) != 0:
            for ax in t['axes']:
                counts[ax] = counts.get(ax, 0) + 1
    # Label is in boundary if and only if it appears exactly once in the subset
    return {ax for ax, c in counts.items() if c == 1}

def merge_cost(tracker_dict, left_mask, right_mask, dimensions):
    b_left = tracker_dict[left_mask]
    b_right = tracker_dict[right_mask]
    union_labels = b_left | b_right
    if not union_labels:
        return 1
    cost = 1
    for lbl in union_labels:
        cost *= dimensions[lbl]
    return cost

def exhaustive_optimum(network):
    """Brute force exhaustive binary tree search for small networks (N <= 6)."""
    tensors = network['tensors']
    dimensions = network['dimensions']
    n = len(tensors)
    if n == 1:
        return 0, [0]

    # Precompute all boundary sets
    tracker = {}
    for mask in range(1, 1 << n):
        tracker[mask] = compute_boundary(tensors, mask)

    # DP over subsets
    dp = {}
    best_split = {}

    for i in range(n):
        dp[1 << i] = 0

    for k in range(2, n + 1):
        for S in range(1, 1 << n):
            if bin(S).count('1') != k:
                continue
            min_cost = None
            chosen_pair = None
            fixed_bit = S & (-S)

            # Enumerate submasks containing fixed_bit
            sub = (S - 1) & S
            while sub > 0:
                if (sub & fixed_bit) != 0 and sub != S:
                    A = sub
                    B = S ^ A
                    step = merge_cost(tracker, A, B, dimensions)
                    tot = dp[A] + dp[B] + step
                    if min_cost is None or tot < min_cost:
                        min_cost = tot
                        chosen_pair = (A, B)
                    elif tot == min_cost:
                        # Tie breaking
                        if A < chosen_pair[0]:
                            chosen_pair = (A, B)
                sub = (sub - 1) & S

            dp[S] = min_cost
            best_split[S] = chosen_pair

    full_mask = (1 << n) - 1
    return dp[full_mask]

def main():
    if len(sys.argv) < 2:
        print("Usage: oracle.py <network.json>")
        sys.exit(1)
    with open(sys.argv[1]) as f:
        data = json.load(f)
    cost = exhaustive_optimum(data)
    print(json.dumps({"optimalCost": str(cost)}))

if __name__ == '__main__':
    main()
