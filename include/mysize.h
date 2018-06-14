#ifndef MYSIZE_H
#define MYSIZE_H
template <class T, std::size_t N>
constexpr std::size_t size(const T (&array)[N]) noexcept
{
    return N;
}
#endif
