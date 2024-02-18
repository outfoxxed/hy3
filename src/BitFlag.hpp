#ifndef BITFLAG
#define BITFLAG

template <typename FlagType>
struct BitFlag
{
    int m_FlagValue = 0;

    BitFlag() = default;

    BitFlag(FlagType flag) {
        m_FlagValue = (int)flag;
    }

    operator FlagType() const {
        return static_cast<FlagType>(m_FlagValue);
    }

    void Set(FlagType flag) {
        m_FlagValue |= (int)flag;
    }

    void Unset(FlagType flag) {
        m_FlagValue &= ~(int)flag;
    }

    void Toggle(FlagType flag) {
        m_FlagValue ^= (int)flag;
    }

    void Mask(FlagType flag) {
        m_FlagValue &= (int)flag;
    }

    bool Has(FlagType flag) const {
        return (m_FlagValue & (int)flag) == (int)flag;
    }

    bool HasAny(FlagType flag) const {
        return (m_FlagValue & (int)flag) != 0;
    }
    bool HasNot(FlagType flag) const {
        return (m_FlagValue & (int)flag) != (int)flag;
    }

    const BitFlag& operator |=(FlagType flag) {
        Set(flag);
        return *this;
    }

    const BitFlag& operator &=(FlagType flag) {
        Mask(flag);
        return *this;
    }

    const BitFlag& operator ^=(FlagType flag) {
        Toggle(flag);
        return *this;
    }

    bool operator==(FlagType flag) const {
        return m_FlagValue == (int)flag;
    }

    bool operator!=(FlagType flag) const {
        return m_FlagValue != (int)flag;
    }
};
#endif