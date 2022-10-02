DEBUG = True

def normalize_score(scores):
    '''
    Input:
        scores: list<float>, size=candidate_num, score for all the candidates
    Return:
        Return the normalized score: list<float>, size=candidate_num
            the best candidate (with the highest original score) gets the maximum normalized score, i.e., len(scores)-1
            the worst candidate (with the lowest original score) gets 0 in normalized score
    '''
    sorted_scores = sorted(scores)
    result = []
    for cand in scores:
        result.append(sorted_scores.index(cand))
    return result
    
def compute_weighted_score(all_scores, weight):
    '''
    all_scores: list<list<float>>, expert_num * candidate_num;
    weight: list<float>, expert_num
    '''
    assert(len(all_scores) == len(weight))
    result = [0 for kkk in range(len(all_scores[0]))] # 0 * candidate_num
    for i in range(len(all_scores)): # for each expert
        assert(len(all_scores[i]) == len(all_scores[0])) # there should always be candidate_num scores
        for j in range(len(all_scores[i])): # for each candidate's score
            result[j] += all_scores[i][j] * weight[i]
    return result


def compute_expert_score(best_index, normalized_all_scores):
    '''
    Input:
        best_index: int, best candidate's index
        normalized_all_scores: list<list<float>>, size = expert_num * candidate_num
    Return:
        return the score of each expert: list<int>, size = expert_num
        The bigger the score is, the better the expert is
        Intuition to compute expert's score is by 
            get normalized_score[best_index] in each expert
            if normalized_score[best_index] also scores the highest, then the expert is perfect
                it gets the score normalized_score[best_index], the highest number
            if normalized_score[best_index] scores the lowest, then it's a worst expert
                it gets the score normalized_score[best_index], the lowest number
            if normalized_score[best_index] is X, then the expert get normalized_score[best_index]
    '''
    expert_num = len(normalized_all_scores)
    candidate_num = len(normalized_all_scores[0])
    assert(best_index < candidate_num)
    result = []
    for i in range(expert_num):
        result.append(normalized_all_scores[i][best_index])
    return result

def normalize_expert_score(expert_score):
    '''
    Normalize the scores X_i to [-1, 1]:
        k = 2 / (max{X_i} - min{X_i})
        normalized_X_i = (X_i - min{X_i}) * k + (-1)
    '''
    max_x_i = max(expert_score)
    min_x_i = min(expert_score)
    k = 2 / (max_x_i - min_x_i)
    result = []
    for x_i in expert_score:
        result.append((x_i - min_x_i) * k + (-1))
    return result

def update_expert_weight(cur_weight, normalized_expert_score, delta):
    '''
    Input:
        cur_weight: list<float>, size = exert_num, representing the weight for each expert
        normalized_expert_score: list<float>, size = exert_num, should be within [-1, 1]
        delta: the maximum allowed weight change
    '''
    assert(len(cur_weight) == len(normalized_expert_score))
    result = []
    for i in range(len(cur_weight)):
        result.append(cur_weight[i] + normalized_expert_score[i] * delta)
    return result

def weighted_expert(expert_num, candidate_num, all_scores, cur_weight, delta):
    '''
    Input:
        expert_num: int, the number of experts (i.e., objectives) we have
        candidate_num: int, the number of candidate circuits
        all_scores: list<list<float>>, size = expert_num * candidate_num; 
            all_scores[i] (0 <= i < expert_num) is a candidate_num-sized list, containing scores for all candidate_num circuits
        cur_weight: list<float>, size = expert_num
            the sum of cur_weight should be 1
        delta: one float, a hyper-parameter specifying the maximum of weight change for each expert
    Return:
        best_score: float, the best circuit's weighted normalized score
        best_index: int, the best circuit's index
        best_expert: int, the best expert's index
        updated_weight: list<float>, to replace cur_weight in the next iteration
    '''
    assert(len(all_scores) == expert_num)
    assert(len(all_scores[0]) == candidate_num)
    assert(len(cur_weight) == expert_num)
    assert(sum(cur_weight) == 1)
    norm_all_scores = []
    for i in range(len(all_scores)):
        norm_all_scores.append(normalize_score(all_scores[i]))

    if DEBUG:
        print(norm_all_scores)
    weighted_score = compute_weighted_score(norm_all_scores, cur_weight)

    if DEBUG:
        print(weighted_score)

    best_score = max(weighted_score)
    best_index = weighted_score.index(best_score)

    if DEBUG:
        print(best_score, best_index)
    
    expert_score = compute_expert_score(best_index, norm_all_scores)
    expert_score_norm = normalize_expert_score(expert_score)
    best_expert = expert_score_norm.index(max(expert_score_norm))

    if DEBUG:
        print(expert_score_norm)

    updated_expert_weight = update_expert_weight(cur_weight, expert_score_norm, delta)
    
    return best_score, best_index, best_expert, updated_expert_weight

if __name__ == "__main__":
    # 3 * 4 matrix
    all_scores = [[1, 1, 1, 2],[2, 10, 3, 4], [-1, 100, 13, -1]]
    cur_weight = [1/3, 1/3, 1/3]
    print(weighted_expert(3, 4, all_scores, cur_weight, 0.1))
    # The normalized scores (intermediate results) will be [[0, 0, 0, 3], [0, 3, 1, 2], [0, 3, 2, 0]]
    # the second candidate is the best, and the second and third expert are equally good experts
    # weighted scores: [0.0, 2.0, 1.0, 1.6666666666666665]
    # best_score = 2.0, best_index=1
    # expert_score_norm: [-1.0, 1.0, 1.0]
    # updated expert score: [0.2333333333333333, 0.43333333333333335, 0.43333333333333335]
