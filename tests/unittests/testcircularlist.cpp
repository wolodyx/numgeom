#include <gtest/gtest.h>

#include <numeric>

#include "numgeom/circularlist.h"


TEST(CircularList, PushBack)
{
    CircularList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    ASSERT_EQ(list.size(), 3);
    ASSERT_EQ(list.front(), 1);
    ASSERT_EQ(list.back(), 3);
    auto it = list.begin();
    ASSERT_EQ((*it), 1); ++it;
    ASSERT_EQ((*it), 2); ++it;
    ASSERT_EQ((*it), 3); ++it;
    ASSERT_EQ(it, list.end());

    auto node = list.firstNode();
    ASSERT_EQ(node->data, 1);
    node = node->next;
    ASSERT_EQ(node->data, 2);
    node = node->next;
    ASSERT_EQ(node->data, 3);
    ASSERT_EQ(node->next, list.firstNode());
}


TEST(CircularList, PassNodes)
{
    CircularList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    list.push_back(4);

    int sum;

    // Поддрежка обхода по диапазону.
    sum = 0;
    for(int i : list)
        sum += i;
    ASSERT_EQ(sum, 10);

    // Демонстрация совместимости итераторов требованиям STL.
    sum = std::accumulate(list.begin(), list.end(), 0);
    ASSERT_EQ(sum, 10);

    // Обходим список два раза, воспользовавшись круговым итерированием.
    sum = 0;
    auto it = list.begin();
    for(int i = 0; i < 8; ++i, ++it)
        sum += (*it);
    ASSERT_EQ(sum, 20);
}


TEST(CircularList, Assign)
{
    std::vector<int> vector = {1, 2, 3, 4};
    CircularList<int> list;
    list.assign(vector.begin(), vector.end());

    ASSERT_EQ(std::accumulate(list.begin(),list.end(),0), 10);
    ASSERT_EQ(*(--list.begin()), 4);
}
