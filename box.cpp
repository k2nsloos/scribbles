#include <set>
#include <array>
#include <iostream>
#include <algorithm>

enum Face
{
    FACE_KING,
    FACE_QUEEN,
    FACE_JOKER
};

enum Color
{
    COLOR_WHITE,
    COLOR_BLACK,
    COLOR_RED
};

struct Card
{
    Face face;
    Color color;
};

const char* const face_labels[] = { "king", "queen", "joker" };
const char* const color_labels[] = { "white", "black", "red" };


static constexpr size_t color_count = 3;
static constexpr size_t box_count = color_count;
static constexpr size_t card_count = color_count;
static constexpr size_t face_permutation_count = 6;

static std::array<std::array<Face, box_count>, face_permutation_count> face_permuations = {
    std::array<Face, box_count>{FACE_KING, FACE_QUEEN, FACE_JOKER},
    std::array<Face, box_count>{FACE_KING, FACE_JOKER, FACE_QUEEN},
    std::array<Face, box_count>{FACE_QUEEN, FACE_KING, FACE_JOKER},
    std::array<Face, box_count>{FACE_QUEEN, FACE_JOKER, FACE_KING}, 
    std::array<Face, box_count>{FACE_JOKER, FACE_KING, FACE_QUEEN},
    std::array<Face, box_count>{FACE_JOKER, FACE_QUEEN, FACE_KING},
};

typedef std::array<Face, card_count> Box;
typedef std::array<Box, box_count> StateType;


static StateType construct(size_t state_id)
{
    StateType elem;
    for (size_t box = 0; box < box_count; ++box) {
        elem[box] = face_permuations[state_id % face_permutation_count];
        state_id /= face_permutation_count;
    }
    return elem;
}

static Color find_color_of_face(const Box& box, Face face)
{
    auto it = std::find(box.cbegin(), box.cend(), face);
    return static_cast<Color>(it - box.cbegin());
}

static bool check_joker_in_the_black_box_shares_color_with_the_king_in_the_white_box(const StateType& s)
{
    const auto& black_box = s[COLOR_BLACK];
    const auto& white_box = s[COLOR_WHITE];
    
    auto joker_color = find_color_of_face(black_box, FACE_JOKER);
    auto king_color = find_color_of_face(white_box, FACE_KING);

    return joker_color == king_color;
}

static bool check_queen_based_joker_does_not_match_white_king(const StateType& s)
{
    const auto& white_box = s[COLOR_WHITE];
    auto queen_color = find_color_of_face(white_box, FACE_QUEEN);

    const auto& box_same_queen_color = s[queen_color];
    auto joker_color = find_color_of_face(box_same_queen_color, FACE_JOKER);
    
    for (size_t box_color = 0; box_color < box_count; ++box_color) {
        if (s[box_color][COLOR_WHITE] != FACE_KING) continue;
        if (box_color == joker_color) return false;
    }

    return true;
}

static bool check_no_dups(const StateType& s)
{
    for (size_t box1 = 0; box1 < box_count; ++box1) {
        for (size_t box2 = box1 + 1; box2 < box_count; ++box2) {
            for (size_t color_id = 0; color_id < color_count; ++color_id) {
                if (s[box1][color_id] == s[box2][color_id]) return false;
            }
        }
    }
    return true;
}

static bool check_unique(const StateType& s, Color face_color, Face face)
{
    size_t cnt = 0;
    for (size_t box_color = 0; box_color < box_count; ++box_color) {
      if (s[box_color][face_color] == face) ++cnt;
    }
    return cnt == 1;
}


static bool check_indirect_king_does_not_back_queen_block_box(const StateType& s)
{
    const auto& red_box = s[COLOR_RED];
    const auto& black_box = s[COLOR_BLACK];

    auto joker_color = find_color_of_face(red_box, FACE_JOKER);
    auto queen_color = find_color_of_face(black_box, FACE_QUEEN);

    const auto& box_same_joker_color = s[joker_color];
    auto king_color = find_color_of_face(box_same_joker_color, FACE_KING);

    return king_color != queen_color;
}


static std::set<StateType> generate_base_set()
{
    std::set<StateType> result;
    size_t max_state_id = 1;
    for (size_t i = 0; i < 9; ++i) max_state_id *= 3;

    for (size_t state_id = 0; state_id < max_state_id; ++state_id)
    {
        auto elem = construct(state_id);
        //if (!has_red_joker(elem)) continue;
        //if (!check_no_dups(elem)) continue;
        if (!check_unique(elem, COLOR_WHITE, FACE_KING)) continue;
        if (!check_unique(elem, COLOR_RED, FACE_JOKER)) continue;
        if (!check_joker_in_the_black_box_shares_color_with_the_king_in_the_white_box(elem)) continue;
        if (!check_queen_based_joker_does_not_match_white_king(elem)) continue;
        if (!check_indirect_king_does_not_back_queen_block_box(elem)) continue;

        result.insert(std::move(elem));
    }

    return result;
}


int main()
{
    auto result = generate_base_set();
    std::cout << result.size() << std::endl;

    for (const auto& elem : result)
    {
        std::cout << (check_no_dups(elem) ? "*" : " ");
        for (size_t box = 0; box < 3; ++box) {
            if (box != 0) std::cout << ",  ";
            std::cout << color_labels[box] << " box = {";
            for (size_t card = 0; card < 3; ++card) {
                if (card != 0) std::cout << ", ";
                std::cout << color_labels[card] << " " << face_labels[elem[box][card]];
            }
            std::cout << "}";
        }
        std::cout << std::endl;
    }

}