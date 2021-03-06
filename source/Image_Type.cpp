#define ENABLE_PPL


#include "Image_Type.h"
#include "Conversion.hpp"


// Functions of class Plane
void Plane::DefaultPara(bool Chroma, value_type _BitDepth, QuantRange _QuantRange)
{
    Quantize_Value(Floor_, Neutral_, Ceil_, _BitDepth, _QuantRange, Chroma);
}

void Plane::CopyParaFrom(const _Myt &src)
{
    Width_ = src.Width();
    Height_ = src.Height();
    PixelCount_ = src.PixelCount();
    BitDepth_ = src.BitDepth();
    Floor_ = src.Floor();
    Neutral_ = src.Neutral();
    Ceil_ = src.Ceil();
    TransferChar_ = src.GetTransferChar();
}

void Plane::InitValue(value_type Value, bool Init)
{
    if (Init)
    {
        Value = Quantize(Value);

        if (Value != 0)
        {
            for_each([&](value_type &x)
            {
                x = Value;
            });
        }
        else
        {
            memset(data(), Value, sizeof(value_type) * size());
        }
    }
}


Plane::Plane(value_type Value, PCType _Width, PCType _Height, value_type _BitDepth, bool Init)
    : _Myt(Value, _Width, _Height, _BitDepth, 0, 0, (value_type(1) << _BitDepth) - 1,
    TransferChar_Default(_Width, _Height, true), Init)
{}

Plane::Plane(value_type Value, PCType _Width, PCType _Height, value_type _BitDepth, value_type _Floor, value_type _Neutral, value_type _Ceil, TransferChar _TransferChar, bool Init)
    : Width_(_Width), Height_(_Height), PixelCount_(_Width * _Height), BitDepth_(_BitDepth),
    Floor_(_Floor), Neutral_(_Neutral), Ceil_(_Ceil), TransferChar_(_TransferChar)
{
    const char *FunctionName = "class Plane constructor";
    if (_BitDepth > MaxBitDepth)
    {
        std::cerr << FunctionName << ": \"BitDepth=" << _BitDepth << "\" is invalid, maximum allowed bit depth is " << MaxBitDepth << ".\n";
        DEBUG_BREAK;
    }
    if (_Ceil <= _Floor)
    {
        std::cerr << FunctionName << ": invalid values of \"Floor=" << _Floor << "\" and \"Ceil=" << _Ceil << "\" are set.\n";
        DEBUG_BREAK;
    }
    if (ValueRange() >= value_type(1) << _BitDepth)
    {
        std::cerr << FunctionName << ": \"Ceil-Floor=" << ValueRange() << "\" exceeds \"BitDepth=" << _BitDepth << "\" limit.\n";
        DEBUG_BREAK;
    }
    if (_Neutral > _Floor && _Neutral != (_Floor + _Ceil + 1) / 2)
    {
        std::cerr << FunctionName << ": invalid values of \"Floor=" << _Floor << "\", \"Neutral=" << _Neutral << "\" and \"Ceil=" << _Ceil << "\" are set.\n";
        DEBUG_BREAK;
    }

    AlignedMalloc(Data_, size());

    InitValue(Value, Init);
}

Plane::Plane(const _Myt &src)
    : _Myt(src, false)
{
    memcpy(data(), src.data(), sizeof(value_type) * size());
}

Plane::Plane(const _Myt &src, bool Init, value_type Value)
    : _Myt(Value, src.Width(), src.Height(), src.BitDepth(), src.Floor(), src.Neutral(), src.Ceil(), src.GetTransferChar(), Init)
{}

Plane::Plane(_Myt &&src)
    : Width_(src.Width()), Height_(src.Height()), PixelCount_(src.PixelCount()), BitDepth_(src.BitDepth()),
    Floor_(src.Floor()), Neutral_(src.Neutral()), Ceil_(src.Ceil()), TransferChar_(src.GetTransferChar())
{
    Data_ = src.data();

    src.Width_ = 0;
    src.Height_ = 0;
    src.PixelCount_ = 0;
    src.Data_ = nullptr;
}

Plane::Plane(const Plane_FL &src, value_type _BitDepth)
    : _Myt(src, _BitDepth, 0, 0, (value_type(1) << _BitDepth) - 1)
{}

Plane::Plane(const Plane_FL &src, value_type _BitDepth, value_type _Floor, value_type _Neutral, value_type _Ceil)
    : _Myt(src, false, 0, _BitDepth, _Floor, _Neutral, _Ceil)
{
    RangeConvert(*this, src);
}

Plane::Plane(const Plane_FL &src, bool Init, value_type Value, value_type _BitDepth)
    : _Myt(src, Init, Value, _BitDepth, 0, 0, (value_type(1) << _BitDepth) - 1)
{}

Plane::Plane(const Plane_FL &src, bool Init, value_type Value, value_type _BitDepth, value_type _Floor, value_type _Neutral, value_type _Ceil)
    : _Myt(Value, src.Width(), src.Height(), _BitDepth, _Floor, _Neutral, _Ceil, src.GetTransferChar(), Init)
{}


Plane::~Plane()
{
    AlignedFree(Data_);
}


Plane &Plane::operator=(const _Myt &src)
{
    if (this == &src)
    {
        return *this;
    }

    CopyParaFrom(src);

    AlignedFree(Data_);
    AlignedMalloc(Data_, size());

    memcpy(data(), src.data(), sizeof(value_type) * size());

    return *this;
}

Plane &Plane::operator=(_Myt &&src)
{
    if (this == &src)
    {
        return *this;
    }

    CopyParaFrom(src);

    AlignedFree(Data_);
    Data_ = src.data();

    src.Width_ = 0;
    src.Height_ = 0;
    src.PixelCount_ = 0;
    src.Data_ = nullptr;

    return *this;
}

bool Plane::operator==(const _Myt &b) const
{
    if (this == &b)
    {
        return true;
    }

    if (Width() != b.Width() || Height() != b.Height() || PixelCount() != b.PixelCount() || BitDepth() != b.BitDepth() ||
        Floor() != b.Floor() || Neutral() != b.Neutral() || Ceil() != b.Ceil() || GetTransferChar() != b.GetTransferChar())
    {
        return false;
    }

    PCType i;
    for (i = 0; Data_[i] == b.Data_[i] && i < PixelCount(); i++);
    if (i < PixelCount())
    {
        return false;
    }
    else
    {
        return true;
    }
}


Plane::value_type Plane::Min() const
{
    return GetMin(*this);
}

Plane::value_type Plane::Max() const
{
    return GetMax(*this);
}

void Plane::MinMax(reference min, reference max) const
{
    GetMinMax(*this, min, max);
}

FLType Plane::Mean() const
{
    uint64 Sum = 0;

    for_each([&](value_type x)
    {
        Sum += x;
    });

    return static_cast<FLType>(Sum) / PixelCount();
}

FLType Plane::Variance(FLType Mean) const
{
    FLType diff;
    FLType Sum = 0;

    for_each([&](value_type x)
    {
        diff = x - Mean;
        Sum += diff * diff;
    });

    return Sum / PixelCount();
}


Plane &Plane::ReSize(PCType _Width, PCType _Height)
{
    if (Width() != _Width || Height() != _Height)
    {
        auto newPC = _Width * _Height;

        if (PixelCount() != newPC)
        {
            auto originPC = PixelCount();
            PixelCount_ = newPC;

            if (newPC == 0)
            {
                AlignedFree(Data_);
            }
            else if (originPC == 0)
            {
                AlignedMalloc(Data_, newPC);
            }
            else
            {
                AlignedRealloc(Data_, newPC);
            }
        }

        Width_ = _Width;
        Height_ = _Height;
    }

    return *this;
}

Plane &Plane::ReQuantize(value_type _BitDepth, QuantRange _QuantRange, bool scale, bool clip)
{
    const char *FunctionName = "Plane::ReQuantize";
    if (_BitDepth > MaxBitDepth)
    {
        std::cerr << FunctionName << ": \"BitDepth=" << _BitDepth
            << "\" is invalid, maximum allowed bit depth is " << MaxBitDepth << ".\n";
        DEBUG_BREAK;
    }

    value_type _Floor, _Neutral, _Ceil;
    Quantize_Value(_Floor, _Neutral, _Ceil, _BitDepth, _QuantRange, isChroma());
    return ReQuantize(_BitDepth, _Floor, _Neutral, _Ceil, scale, clip);
}

Plane &Plane::ReQuantize(value_type _BitDepth, value_type _Floor, value_type _Neutral, value_type _Ceil, bool scale, bool clip)
{
    value_type _ValueRange = _Ceil - _Floor;

    const char *FunctionName = "Plane::ReQuantize";
    if (_BitDepth > MaxBitDepth)
    {
        std::cerr << FunctionName << ": \"BitDepth=" << _BitDepth
            << "\" is invalid, maximum allowed bit depth is " << MaxBitDepth << ".\n";
        DEBUG_BREAK;
    }
    if (_Ceil <= _Floor)
    {
        std::cerr << FunctionName << ": invalid values of \"Floor="
            << _Floor << "\" and \"Ceil=" << _Ceil << "\" are set.\n";
        DEBUG_BREAK;
    }
    if (_ValueRange >= value_type(1) << _BitDepth)
    {
        std::cerr << FunctionName << ": \"Ceil-Floor=" << _ValueRange
            << "\" exceeds \"BitDepth=" << _BitDepth << "\" limit.\n";
        DEBUG_BREAK;
    }
    if (_Neutral > _Floor && _Neutral != (_Floor + _Ceil + 1) / 2)
    {
        std::cerr << FunctionName << ": invalid values of \"Floor=" << _Floor
            << "\", \"Neutral=" << _Neutral << "\" and \"Ceil=" << _Ceil << "\" are set.\n";
        DEBUG_BREAK;
    }

    if (scale && data() && (Floor() != _Floor || Neutral() != _Neutral || Ceil() != _Ceil))
    {
        FLType gain = static_cast<FLType>(_ValueRange) / ValueRange();
        FLType offset = _Neutral - Neutral() * gain + FLType(_Floor < _Neutral && (_Floor + _Ceil) % 2 == 1 ? 0.499999 : 0.5);

        if (clip)
        {
            FLType FloorFL = static_cast<FLType>(_Floor);
            FLType CeilFL = static_cast<FLType>(_Ceil);

            transform([&](value_type x)
            {
                return static_cast<value_type>(Clip(x * gain + offset, FloorFL, CeilFL));
            });
        }
        else
        {
            transform([&](value_type x)
            {
                return static_cast<value_type>(x * gain + offset);
            });
        }
    }

    BitDepth_ = _BitDepth;
    Floor_ = _Floor;
    Neutral_ = _Neutral;
    Ceil_ = _Ceil;

    return *this;
}


Plane &Plane::Binarize(const _Myt &src, value_type lower_thrD, value_type upper_thrD)
{
    double lower_thr = static_cast<double>(lower_thrD - src.Floor()) / src.ValueRange();
    double upper_thr = static_cast<double>(upper_thrD - src.Floor()) / src.ValueRange();

    return Binarize_ratio(src, lower_thr, upper_thr);
}

Plane &Plane::Binarize_ratio(const _Myt &src, double lower_thr, double upper_thr)
{
    auto &dst = *this;

    value_type lower_thrD = static_cast<value_type>(lower_thr * src.ValueRange() + 0.5) + src.Floor();
    value_type upper_thrD = static_cast<value_type>(upper_thr * src.ValueRange() + 0.5) + src.Floor();

    if (upper_thr <= lower_thr || lower_thr >= 1 || upper_thr < 0)
    {
        dst.for_each([&](value_type &x)
        {
            x = dst.Floor();
        });
    }
    else if (lower_thr < 0)
    {
        if (upper_thr >= 1)
        {
            dst.for_each([&](value_type &x)
            {
                x = dst.Ceil();
            });
        }
        else
        {
            dst.transform([&](value_type x)
            {
                return x <= upper_thrD ? dst.Ceil() : dst.Floor();
            });
        }
    }
    else
    {
        if (upper_thr >= 1)
        {
            dst.transform([&](value_type x)
            {
                return x > lower_thrD ? dst.Ceil() : dst.Floor();
            });
        }
        else
        {
            dst.transform([&](value_type x)
            {
                return x > lower_thrD && x <= upper_thrD ? dst.Ceil() : dst.Floor();
            });
        }
    }

    return *this;
}


// Functions of class Plane_FL
void Plane_FL::DefaultPara(bool Chroma, value_type range)
{
    if (Chroma) // Plane "src" is chroma
    {
        Floor_ = -range / 2;
        Neutral_ = 0;
        Ceil_ = range / 2;
    }
    else // Plane "src" is not chroma
    {
        Floor_ = 0;
        Neutral_ = 0;
        Ceil_ = range;
    }
}

void Plane_FL::CopyParaFrom(const _Myt &src)
{
    Width_ = src.Width();
    Height_ = src.Height();
    PixelCount_ = src.PixelCount();
    Floor_ = src.Floor();
    Neutral_ = src.Neutral();
    Ceil_ = src.Ceil();
    TransferChar_ = src.GetTransferChar();
}

void Plane_FL::InitValue(value_type Value, bool Init)
{
    if (Init)
    {
        Value = Quantize(Value);
        for_each([&](value_type &x)
        {
            x = Value;
        });
    }
}


Plane_FL::Plane_FL(value_type Value, PCType _Width, PCType _Height, bool RGB, bool Chroma, bool Init)
    : Width_(_Width), Height_(_Height), PixelCount_(_Width * _Height), TransferChar_(!RGB&&Chroma ? TransferChar::linear : TransferChar_Default(_Width, _Height, RGB))
{
    DefaultPara(!RGB&&Chroma);

    AlignedMalloc(Data_, size());

    InitValue(Value, Init);
}

Plane_FL::Plane_FL(value_type Value, PCType _Width, PCType _Height, value_type _Floor, value_type _Neutral, value_type _Ceil, TransferChar _TransferChar, bool Init)
    : Width_(_Width), Height_(_Height), PixelCount_(_Width * _Height),
    Floor_(_Floor), Neutral_(_Neutral), Ceil_(_Ceil), TransferChar_(_TransferChar)
{
    AlignedMalloc(Data_, size());

    InitValue(Value, Init);
}

Plane_FL::Plane_FL(const _Myt &src)
    : _Myt(src, false)
{
    memcpy(data(), src.data(), sizeof(value_type) * size());
}

Plane_FL::Plane_FL(const _Myt &src, bool Init, value_type Value)
    : _Myt(Value, src.Width(), src.Height(), src.Floor(), src.Neutral(), src.Ceil(), src.GetTransferChar(), Init)
{}

Plane_FL::Plane_FL(_Myt &&src)
    : Width_(src.Width()), Height_(src.Height()), PixelCount_(src.PixelCount()),
    Floor_(src.Floor()), Neutral_(src.Neutral()), Ceil_(src.Ceil()), TransferChar_(src.GetTransferChar())
{
    Data_ = src.data();

    src.Width_ = 0;
    src.Height_ = 0;
    src.PixelCount_ = 0;
    src.Data_ = nullptr;
}

Plane_FL::Plane_FL(const Plane &src, value_type range)
    : _Myt(src, false, 0, range)
{
    RangeConvert(*this, src);
}

Plane_FL::Plane_FL(const Plane &src, bool Init, value_type Value, value_type range)
    : Width_(src.Width()), Height_(src.Height()), PixelCount_(src.PixelCount()), TransferChar_(src.GetTransferChar())
{
    AlignedMalloc(Data_, size());

    if (range > 0)
    {
        DefaultPara(src.isChroma(), range);
    }
    else
    {
        Floor_ = static_cast<value_type>(src.Floor());
        Neutral_ = static_cast<value_type>(src.Neutral());
        Ceil_ = static_cast<value_type>(src.Ceil());
    }

    InitValue(Value, Init);
}


Plane_FL::~Plane_FL()
{
    AlignedFree(Data_);
}


Plane_FL &Plane_FL::operator=(const _Myt &src)
{
    if (this == &src)
    {
        return *this;
    }

    CopyParaFrom(src);

    AlignedFree(Data_);
    AlignedMalloc(Data_, size());

    memcpy(data(), src.data(), sizeof(value_type) * size());

    return *this;
}

Plane_FL &Plane_FL::operator=(_Myt &&src)
{
    if (this == &src)
    {
        return *this;
    }

    CopyParaFrom(src);

    AlignedFree(Data_);
    Data_ = src.data();

    src.Width_ = 0;
    src.Height_ = 0;
    src.PixelCount_ = 0;
    src.Data_ = nullptr;

    return *this;
}

bool Plane_FL::operator==(const _Myt &b) const
{
    if (this == &b)
    {
        return true;
    }

    if (Width() != b.Width() || Height() != b.Height() || PixelCount() != b.PixelCount() ||
        Floor() != b.Floor() || Neutral() != b.Neutral() || Ceil() != b.Ceil() || GetTransferChar() != b.GetTransferChar())
    {
        return false;
    }

    PCType i;
    for (i = 0; Data_[i] == b.Data_[i] && i < PixelCount(); i++);
    if (i < PixelCount())
    {
        return false;
    }
    else
    {
        return true;
    }
}


Plane_FL::value_type Plane_FL::Min() const
{
    return GetMin(*this);
}

Plane_FL::value_type Plane_FL::Max() const
{
    return GetMax(*this);
}

void Plane_FL::MinMax(reference min, reference max) const
{
    GetMinMax(*this, min, max);
}

Plane_FL::value_type Plane_FL::Mean() const
{
    value_type Sum = 0;

    for_each([&](value_type x)
    {
        Sum += x;
    });

    return Sum / PixelCount();
}

Plane_FL::value_type Plane_FL::Variance(value_type Mean) const
{
    value_type diff;
    value_type Sum = 0;

    for_each([&](value_type x)
    {
        diff = x - Mean;
        Sum += diff * diff;
    });

    return Sum / PixelCount();
}


Plane_FL &Plane_FL::ReSize(PCType _Width, PCType _Height)
{
    if (Width() != _Width || Height() != _Height)
    {
        auto newPC = _Width * _Height;

        if (PixelCount() != newPC)
        {
            auto originPC = PixelCount();
            PixelCount_ = newPC;

            if (newPC == 0)
            {
                AlignedFree(Data_);
            }
            else if (originPC == 0)
            {
                AlignedMalloc(Data_, newPC);
            }
            else
            {
                AlignedRealloc(Data_, newPC);
            }
        }

        Width_ = _Width;
        Height_ = _Height;
    }

    return *this;
}

Plane_FL &Plane_FL::ReQuantize(value_type _Floor, value_type _Neutral, value_type _Ceil, bool scale, bool clip)
{
    PCType i;

    const char *FunctionName = "Plane_FL::ReQuantize";
    if (_Ceil <= _Floor)
    {
        std::cerr << FunctionName << ": invalid values of \"Floor="
            << _Floor << "\" and \"Ceil=" << _Ceil << "\" are set.\n";
        DEBUG_BREAK;
    }
    if (_Neutral > _Floor && _Neutral != (_Floor + _Ceil) / 2.)
    {
        std::cerr << FunctionName << ": invalid values of \"Floor=" << _Floor
            << "\", \"Neutral=" << _Neutral << "\" and \"Ceil=" << _Ceil << "\" are set.\n";
        DEBUG_BREAK;
    }

    if (scale && data() && (Floor() != _Floor || Neutral() != _Neutral || Ceil() != _Ceil))
    {
        value_type gain = (_Ceil - _Floor) / ValueRange();
        value_type offset = _Neutral - Neutral() * gain;

        if (clip)
        {
            for (i = 0; i < PixelCount(); i++)
            {
                Data_[i] = Clip(Data_[i] * gain + offset, _Floor, _Ceil);
            }
        }
        else
        {
            for (i = 0; i < PixelCount(); i++)
            {
                Data_[i] = Data_[i] * gain + offset;
            }
        }
    }

    Floor_ = _Floor;
    Neutral_ = _Neutral;
    Ceil_ = _Ceil;

    return *this;
}


Plane_FL &Plane_FL::Binarize(const _Myt &src, value_type lower_thrD, value_type upper_thrD)
{
    PCType i, j, upper;

    PCType height = src.Height();
    PCType width = src.Width();
    PCType stride = src.Stride();

    double lower_thr = static_cast<double>(lower_thrD - src.Floor()) / src.ValueRange();
    double upper_thr = static_cast<double>(upper_thrD - src.Floor()) / src.ValueRange();

    if (upper_thr <= lower_thr || lower_thr >= 1 || upper_thr < 0)
    {
        for (j = 0; j < height; j++)
        {
            i = stride * j;
            for (upper = i + width; i < upper; i++)
            {
                Data_[i] = Floor();
            }
        }
    }
    else if (lower_thr < 0)
    {
        if (upper_thr >= 1)
        {
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    Data_[i] = Ceil();
                }
            }
        }
        else
        {
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    Data_[i] = src[i] <= upper_thrD ? Ceil() : Floor();
                }
            }
        }
    }
    else
    {
        if (upper_thr >= 1)
        {
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    Data_[i] = src[i] > lower_thrD ? Ceil() : Floor();
                }
            }
        }
        else
        {
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    Data_[i] = src[i] > lower_thrD && src[i] <= upper_thrD ? Ceil() : Floor();
                }
            }
        }
    }

    return *this;
}

Plane_FL &Plane_FL::Binarize_ratio(const _Myt &src, double lower_thr, double upper_thr)
{
    value_type lower_thrD = static_cast<value_type>(lower_thr * src.ValueRange()) + src.Floor();
    value_type upper_thrD = static_cast<value_type>(upper_thr * src.ValueRange()) + src.Floor();

    return Binarize(src, lower_thrD, upper_thrD);
}


// Functions of class Frame
void Frame::InitPlanes(PCType _Width, PCType _Height, value_type _BitDepth, bool Init)
{
    value_type _Floor, _Neutral, _Ceil;

    FreePlanes();

    if (isRGB())
    {
        Quantize_Value(_Floor, _Neutral, _Ceil, _BitDepth, QuantRange_, false);

        if (PixelType_ == PixelType::RGB || PixelType_ == PixelType::R)
        {
            R_ = new _Mysub(_Floor, _Width, _Height, _BitDepth, _Floor, _Neutral, _Ceil, TransferChar_, Init);

            P_[PlaneCount_++] = R_;
        }

        if (PixelType_ == PixelType::RGB || PixelType_ == PixelType::G)
        {
            G_ = new _Mysub(_Floor, _Width, _Height, _BitDepth, _Floor, _Neutral, _Ceil, TransferChar_, Init);

            P_[PlaneCount_++] = G_;
        }

        if (PixelType_ == PixelType::RGB || PixelType_ == PixelType::B)
        {
            B_ = new _Mysub(_Floor, _Width, _Height, _BitDepth, _Floor, _Neutral, _Ceil, TransferChar_, Init);

            P_[PlaneCount_++] = B_;
        }
    }
    else if (isYUV())
    {
        if (PixelType_ != PixelType::U && PixelType_ != PixelType::V)
        {
            if (PixelType_ == PixelType::YUV422 && _Width % 2)
            {
                _Width = (_Width / 2 + 1) * 2;
            }
            else if (PixelType_ == PixelType::YUV420)
            {
                if (_Width % 2)  _Width = (_Width / 2 + 1) * 2;
                if (_Height % 2) _Height = (_Height / 2 + 1) * 2;
            }
            else if (PixelType_ == PixelType::YUV411 && _Width % 4)
            {
                _Width = (_Width / 4 + 1) * 4;
            }

            Quantize_Value(_Floor, _Neutral, _Ceil, _BitDepth, QuantRange_, false);

            Y_ = new _Mysub(_Floor, _Width, _Height, _BitDepth, _Floor, _Neutral, _Ceil, TransferChar_, Init);

            P_[PlaneCount_++] = Y_;
        }

        if (PixelType_ != PixelType::Y)
        {
            if (PixelType_ == PixelType::YUV422)
            {
                _Width = _Width / 2;
            }
            else if (PixelType_ == PixelType::YUV420)
            {
                _Width = _Width / 2;
                _Height = _Height / 2;
            }
            else if (PixelType_ == PixelType::YUV411)
            {
                _Width = _Width / 4;
            }

            Quantize_Value(_Floor, _Neutral, _Ceil, _BitDepth, QuantRange_, true);

            if (PixelType_ != PixelType::V)
            {
                U_ = new _Mysub(_Neutral, _Width, _Height, _BitDepth, _Floor, _Neutral, _Ceil, TransferChar::linear, Init);

                P_[PlaneCount_++] = U_;
            }

            if (PixelType_ != PixelType::U)
            {
                V_ = new _Mysub(_Neutral, _Width, _Height, _BitDepth, _Floor, _Neutral, _Ceil, TransferChar::linear, Init);

                P_[PlaneCount_++] = V_;
            }
        }
    }
}

void Frame::CopyPlanes(const _Myt &src, bool Copy, bool Init)
{
    value_type _Floor, _Neutral, _Ceil;

    FreePlanes();

    if (isRGB())
    {
        if (PixelType_ == PixelType::RGB || PixelType_ == PixelType::R)
        {
            if (Copy)
            {
                R_ = new _Mysub(src.R());
            }
            else
            {
                Quantize_Value(_Floor, _Neutral, _Ceil, src.R().BitDepth(), QuantRange_, false);

                R_ = new _Mysub(_Floor, src.R().Width(), src.R().Height(), src.R().BitDepth(), _Floor, _Neutral, _Ceil, TransferChar_, Init);
            }
            P_[PlaneCount_++] = R_;
        }

        if (PixelType_ == PixelType::RGB || PixelType_ == PixelType::G)
        {
            if (Copy)
            {
                G_ = new _Mysub(src.G());
            }
            else
            {
                Quantize_Value(_Floor, _Neutral, _Ceil, src.G().BitDepth(), QuantRange_, false);

                G_ = new _Mysub(_Floor, src.G().Width(), src.G().Height(), src.G().BitDepth(), _Floor, _Neutral, _Ceil, TransferChar_, Init);
            }
            P_[PlaneCount_++] = G_;
        }

        if (PixelType_ == PixelType::RGB || PixelType_ == PixelType::B)
        {
            if (Copy)
            {
                B_ = new _Mysub(src.B());
            }
            else
            {
                Quantize_Value(_Floor, _Neutral, _Ceil, src.B().BitDepth(), QuantRange_, false);

                B_ = new _Mysub(_Floor, src.B().Width(), src.B().Height(), src.B().BitDepth(), _Floor, _Neutral, _Ceil, TransferChar_, Init);
            }
            P_[PlaneCount_++] = B_;
        }
    }
    else if (isYUV())
    {
        if (PixelType_ != PixelType::U && PixelType_ != PixelType::V)
        {
            if (Copy)
            {
                Y_ = new _Mysub(src.Y());
            }
            else
            {
                Quantize_Value(_Floor, _Neutral, _Ceil, src.Y().BitDepth(), QuantRange_, false);

                Y_ = new _Mysub(_Floor, src.Y().Width(), src.Y().Height(), src.Y().BitDepth(), _Floor, _Neutral, _Ceil, TransferChar_, Init);
            }
            P_[PlaneCount_++] = Y_;
        }

        if (PixelType_ != PixelType::Y)
        {
            if (PixelType_ != PixelType::V)
            {
                if (Copy)
                {
                    U_ = new _Mysub(src.U());
                }
                else
                {
                    Quantize_Value(_Floor, _Neutral, _Ceil, src.U().BitDepth(), QuantRange_, true);

                    U_ = new _Mysub(_Neutral, src.U().Width(), src.U().Height(), src.U().BitDepth(), _Floor, _Neutral, _Ceil, TransferChar::linear, Init);
                }
                P_[PlaneCount_++] = U_;
            }

            if (PixelType_ != PixelType::U)
            {
                if (Copy)
                {
                    V_ = new _Mysub(src.V());
                }
                else
                {
                    Quantize_Value(_Floor, _Neutral, _Ceil, src.V().BitDepth(), QuantRange_, true);

                    V_ = new _Mysub(_Neutral, src.V().Width(), src.V().Height(), src.V().BitDepth(), _Floor, _Neutral, _Ceil, TransferChar::linear, Init);
                }
                P_[PlaneCount_++] = V_;
            }
        }
    }
}

void Frame::MovePlanes(_Myt &src)
{
    PlaneCount_ = src.PlaneCount_;

    P_ = std::move(src.P_);
    src.P_.insert(src.P_.end(), MaxPlaneCount, nullptr);

    R_ = src.R_;
    G_ = src.G_;
    B_ = src.B_;
    Y_ = src.Y_;
    U_ = src.U_;
    V_ = src.V_;
    A_ = src.A_;

    src.R_ = nullptr;
    src.G_ = nullptr;
    src.B_ = nullptr;
    src.Y_ = nullptr;
    src.U_ = nullptr;
    src.V_ = nullptr;
    src.A_ = nullptr;
}

void Frame::FreePlanes()
{
    PlaneCount_ = 0;

    for (auto x : P_)
    {
        delete x;
        x = nullptr;
    }

    R_ = nullptr;
    G_ = nullptr;
    B_ = nullptr;
    Y_ = nullptr;
    U_ = nullptr;
    V_ = nullptr;
    A_ = nullptr;
}


Frame::Frame(FCType _FrameNum, PixelType _PixelType, PCType _Width, PCType _Height, value_type _BitDepth, bool Init)
    : _Myt(_FrameNum, _PixelType, _Width, _Height, _BitDepth, isYUV(_PixelType) ? QuantRange::TV : QuantRange::PC, ChromaPlacement::MPEG2, Init)
{}

Frame::Frame(FCType _FrameNum, PixelType _PixelType, PCType _Width, PCType _Height, value_type _BitDepth,
    QuantRange _QuantRange, ChromaPlacement _ChromaPlacement, bool Init)
    : _Myt(_FrameNum, _PixelType, _Width, _Height, _BitDepth, _QuantRange, _ChromaPlacement,
    ColorPrim_Default(_Width, _Height, isRGB(_PixelType)), TransferChar_Default(_Width, _Height, isRGB(_PixelType)), ColorMatrix_Default(_Width, _Height))
{}

Frame::Frame(FCType _FrameNum, PixelType _PixelType, PCType _Width, PCType _Height, value_type _BitDepth, QuantRange _QuantRange,
    ChromaPlacement _ChromaPlacement, ColorPrim _ColorPrim, TransferChar _TransferChar, ColorMatrix _ColorMatrix, bool Init)
    : FrameNum_(_FrameNum), PixelType_(_PixelType), QuantRange_(_QuantRange), ChromaPlacement_(_ChromaPlacement),
    ColorPrim_(_ColorPrim), TransferChar_(_TransferChar), ColorMatrix_(_ColorMatrix), P_(MaxPlaneCount, nullptr)
{
    const char *FunctionName = "class Frame constructor";
    if (_BitDepth > MaxBitDepth)
    {
        std::cerr << FunctionName << ": \"BitDepth=" << _BitDepth
            << "\" is invalid, maximum allowed bit depth is " << MaxBitDepth << ".\n";
        DEBUG_BREAK;
    }

    InitPlanes(_Width, _Height, _BitDepth, Init);
}

Frame::Frame(const _Myt &src, bool Copy, bool Init)
    : FrameNum_(src.FrameNum()), PixelType_(src.GetPixelType()), QuantRange_(src.GetQuantRange()), ChromaPlacement_(src.GetChromaPlacement()),
    ColorPrim_(src.GetColorPrim()), TransferChar_(src.GetTransferChar()), ColorMatrix_(src.GetColorMatrix()), P_(MaxPlaneCount, nullptr)
{
    CopyPlanes(src, Copy, Init);
}

Frame::Frame(_Myt &&src)
    : FrameNum_(src.FrameNum()), PixelType_(src.GetPixelType()), QuantRange_(src.GetQuantRange()), ChromaPlacement_(src.GetChromaPlacement()),
    ColorPrim_(src.GetColorPrim()), TransferChar_(src.GetTransferChar()), ColorMatrix_(src.GetColorMatrix())
{
    MovePlanes(src);
}


Frame::~Frame()
{
    FreePlanes();
}


Frame &Frame::operator=(const _Myt &src)
{
    if (this == &src)
    {
        return *this;
    }

    FrameNum_ = src.FrameNum();
    PixelType_ = src.GetPixelType();
    QuantRange_ = src.GetQuantRange();
    ChromaPlacement_ = src.GetChromaPlacement();
    ColorPrim_ = src.GetColorPrim();
    TransferChar_ = src.GetTransferChar();
    ColorMatrix_ = src.GetColorMatrix();

    CopyPlanes(src, true);

    return *this;
}

Frame &Frame::operator=(_Myt &&src)
{
    if (this == &src)
    {
        return *this;
    }

    FrameNum_ = src.FrameNum();
    PixelType_ = src.GetPixelType();
    QuantRange_ = src.GetQuantRange();
    ChromaPlacement_ = src.GetChromaPlacement();
    ColorPrim_ = src.GetColorPrim();
    TransferChar_ = src.GetTransferChar();
    ColorMatrix_ = src.GetColorMatrix();

    FreePlanes();

    MovePlanes(src);

    return *this;
}

bool Frame::operator==(const _Myt &b) const
{
    if (this == &b)
    {
        return true;
    }

    if (FrameNum() != b.FrameNum() || GetPixelType() != b.GetPixelType() || GetQuantRange() != b.GetQuantRange() || GetChromaPlacement() != b.GetChromaPlacement()
        || GetColorPrim() != b.GetColorPrim() || GetTransferChar() != b.GetTransferChar() || GetColorMatrix() != b.GetColorMatrix() || PlaneCount() != b.PlaneCount())
    {
        return false;
    }

    if (isRGB())
    {
        if (PixelType_ == PixelType::RGB || PixelType_ == PixelType::R)
        {
            if (R() != b.R()) return false;
        }

        if (PixelType_ == PixelType::RGB || PixelType_ == PixelType::G)
        {
            if (G() != b.G()) return false;
        }

        if (PixelType_ == PixelType::RGB || PixelType_ == PixelType::B)
        {
            if (B() != b.B()) return false;
        }
    }
    else if (isYUV())
    {
        if (PixelType_ != PixelType::U && PixelType_ != PixelType::V)
        {
            if (Y() != b.Y()) return false;
        }

        if (PixelType_ != PixelType::Y)
        {
            if (PixelType_ != PixelType::V)
            {
                if (U() != b.U()) return false;
            }

            if (PixelType_ != PixelType::U)
            {
                if (V() != b.V()) return false;
            }
        }
    }

    return true;
}
