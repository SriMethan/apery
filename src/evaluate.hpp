/*
  Apery, a USI shogi playing engine derived from Stockfish, a UCI chess playing engine.
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Copyright (C) 2011-2017 Hiraoka Takuya

  Apery is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Apery is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef APERY_EVALUATE_HPP
#define APERY_EVALUATE_HPP

#include "overloadEnumOperators.hpp"
#include "common.hpp"
#include "square.hpp"
#include "piece.hpp"
#include "pieceScore.hpp"
#include "position.hpp"

// 評価関数テーブルのオフセット。
// f_xxx が味方の駒、e_xxx が敵の駒
// Bonanza の影響で持ち駒 0 の場合のインデックスが存在するが、参照する事は無い。
// todo: 持ち駒 0 の位置を詰めてテーブルを少しでも小さくする。(キャッシュに少しは乗りやすい?)
enum EvalIndex {
    f_hand_pawn   = 0, // 0
    e_hand_pawn   = f_hand_pawn   + 19,
    f_hand_lance  = e_hand_pawn   + 19,
    e_hand_lance  = f_hand_lance  +  5,
    f_hand_knight = e_hand_lance  +  5,
    e_hand_knight = f_hand_knight +  5,
    f_hand_silver = e_hand_knight +  5,
    e_hand_silver = f_hand_silver +  5,
    f_hand_gold   = e_hand_silver +  5,
    e_hand_gold   = f_hand_gold   +  5,
    f_hand_bishop = e_hand_gold   +  5,
    e_hand_bishop = f_hand_bishop +  3,
    f_hand_rook   = e_hand_bishop +  3,
    e_hand_rook   = f_hand_rook   +  3,
    fe_hand_end   = e_hand_rook   +  3,

    f_pawn        = fe_hand_end,
    e_pawn        = f_pawn        + 81,
    f_lance       = e_pawn        + 81,
    e_lance       = f_lance       + 81,
    f_knight      = e_lance       + 81,
    e_knight      = f_knight      + 81,
    f_silver      = e_knight      + 81,
    e_silver      = f_silver      + 81,
    f_gold        = e_silver      + 81,
    e_gold        = f_gold        + 81,
    f_bishop      = e_gold        + 81,
    e_bishop      = f_bishop      + 81,
    f_horse       = e_bishop      + 81,
    e_horse       = f_horse       + 81,
    f_rook        = e_horse       + 81,
    e_rook        = f_rook        + 81,
    f_dragon      = e_rook        + 81,
    e_dragon      = f_dragon      + 81,
    fe_end        = e_dragon      + 81
};
OverloadEnumOperators(EvalIndex);

enum EvalIndexOnlyF {
    of_hand_pawn   = 0, // 0
    of_hand_lance  = of_hand_pawn   + 19,
    of_hand_knight = of_hand_lance  +  5,
    of_hand_silver = of_hand_knight +  5,
    of_hand_gold   = of_hand_silver +  5,
    of_hand_bishop = of_hand_gold   +  5,
    of_hand_rook   = of_hand_bishop +  3,
    of_hand_end    = of_hand_rook   +  3,

    of_pawn        = of_hand_end,
    of_lance       = of_pawn        + 81,
    of_knight      = of_lance       + 81,
    of_silver      = of_knight      + 81,
    of_gold        = of_silver      + 81,
    of_bishop      = of_gold        + 81,
    of_horse       = of_bishop      + 81,
    of_rook        = of_horse       + 81,
    of_dragon      = of_rook        + 81,
    of_end         = of_dragon      + 81
};
OverloadEnumOperators(EvalIndexOnlyF);

const int FVScale = 32;

const EvalIndex KPPIndexArray[] = {
    f_hand_pawn, e_hand_pawn, f_hand_lance, e_hand_lance, f_hand_knight,
    e_hand_knight, f_hand_silver, e_hand_silver, f_hand_gold, e_hand_gold,
    f_hand_bishop, e_hand_bishop, f_hand_rook, e_hand_rook, /*fe_hand_end,*/
    f_pawn, e_pawn, f_lance, e_lance, f_knight, e_knight, f_silver, e_silver,
    f_gold, e_gold, f_bishop, e_bishop, f_horse, e_horse, f_rook, e_rook,
    f_dragon, e_dragon, fe_end
};

inline Square kppIndexToSquare(const EvalIndex i) {
    const auto it = std::upper_bound(std::begin(KPPIndexArray), std::end(KPPIndexArray), i);
    return static_cast<Square>(i - *(it - 1));
}
inline EvalIndex kppIndexBegin(const EvalIndex i) {
    return *(std::upper_bound(std::begin(KPPIndexArray), std::end(KPPIndexArray), i) - 1);
}
inline bool kppIndexIsBlack(const EvalIndex i) {
    // f_xxx と e_xxx が交互に配列に格納されているので、インデックスが偶数の時は Black
    return !((std::upper_bound(std::begin(KPPIndexArray), std::end(KPPIndexArray), i) - 1) - std::begin(KPPIndexArray) & 1);
}
inline EvalIndex kppBlackIndexToWhiteBegin(const EvalIndex i) {
    assert(kppIndexIsBlack(i));
    return *std::upper_bound(std::begin(KPPIndexArray), std::end(KPPIndexArray), i);
}
inline EvalIndex kppWhiteIndexToBlackBegin(const EvalIndex i) {
    return *(std::upper_bound(std::begin(KPPIndexArray), std::end(KPPIndexArray), i) - 2);
}
inline EvalIndex kppIndexToOpponentBegin(const EvalIndex i, const bool isBlack) {
    return *(std::upper_bound(std::begin(KPPIndexArray), std::end(KPPIndexArray), i) - static_cast<int>(!isBlack) * 2);
}
inline EvalIndex kppIndexToOpponentBegin(const EvalIndex i) {
    // todo: 高速化
    return kppIndexToOpponentBegin(i, kppIndexIsBlack(i));
}

inline EvalIndex inverseFileIndexIfLefterThanMiddle(const EvalIndex index) {
    if (index < fe_hand_end) return index;
    const auto begin = kppIndexBegin(index);
    const Square sq = static_cast<Square>(index - begin);
    if (sq <= SQ59) return index;
    return static_cast<EvalIndex>(begin + inverseFile(sq));
};
inline EvalIndex inverseFileIndexIfOnBoard(const EvalIndex index) {
    if (index < fe_hand_end) return index;
    const auto begin = kppIndexBegin(index);
    const Square sq = static_cast<Square>(index - begin);
    return static_cast<EvalIndex>(begin + inverseFile(sq));
};
inline EvalIndex inverseFileIndexOnBoard(const EvalIndex index) {
    assert(f_pawn <= index);
    const auto begin = kppIndexBegin(index);
    const Square sq = static_cast<Square>(index - begin);
    return static_cast<EvalIndex>(begin + inverseFile(sq));
};
inline EvalIndex kppWhiteIndexToBlackIndex(const EvalIndex index) {
    const EvalIndex indexBegin = kppIndexBegin(index);
    const EvalIndex blackBegin = kppWhiteIndexToBlackBegin(index);
    return blackBegin + (index < fe_hand_end ? index - indexBegin : (EvalIndex)inverse((Square)(index - indexBegin)));
}
inline EvalIndex kppIndexToOpponentIndex(const EvalIndex index) {
    const EvalIndex indexBegin = kppIndexBegin(index);
    const EvalIndex opponentBegin = kppIndexToOpponentBegin(index);
    return opponentBegin + (index < fe_hand_end ? index - indexBegin : (EvalIndex)inverse((Square)(index - indexBegin)));
}

inline EvalIndexOnlyF evalIndexToEvalIndexOnlyF(const EvalIndex i) {
    assert(kppIndexIsBlack(i));
    const EvalIndex iBegin = kppIndexBegin(i);
    EvalIndexOnlyF ofBegin;
    switch (iBegin) {
    case f_hand_pawn  : ofBegin = of_hand_pawn  ; break;
    case f_hand_lance : ofBegin = of_hand_lance ; break;
    case f_hand_knight: ofBegin = of_hand_knight; break;
    case f_hand_silver: ofBegin = of_hand_silver; break;
    case f_hand_gold  : ofBegin = of_hand_gold  ; break;
    case f_hand_bishop: ofBegin = of_hand_bishop; break;
    case f_hand_rook  : ofBegin = of_hand_rook  ; break;
    case f_pawn       : ofBegin = of_pawn       ; break;
    case f_lance      : ofBegin = of_lance      ; break;
    case f_knight     : ofBegin = of_knight     ; break;
    case f_silver     : ofBegin = of_silver     ; break;
    case f_gold       : ofBegin = of_gold       ; break;
    case f_bishop     : ofBegin = of_bishop     ; break;
    case f_horse      : ofBegin = of_horse      ; break;
    case f_rook       : ofBegin = of_rook       ; break;
    case f_dragon     : ofBegin = of_dragon     ; break;
    default: UNREACHABLE;
    }
    return ofBegin + (i - iBegin);
}

struct KPPBoardIndexStartToPiece : public std::unordered_map<int, Piece> {
    KPPBoardIndexStartToPiece() {
        (*this)[f_pawn  ] = BPawn;
        (*this)[e_pawn  ] = WPawn;
        (*this)[f_lance ] = BLance;
        (*this)[e_lance ] = WLance;
        (*this)[f_knight] = BKnight;
        (*this)[e_knight] = WKnight;
        (*this)[f_silver] = BSilver;
        (*this)[e_silver] = WSilver;
        (*this)[f_gold  ] = BGold;
        (*this)[e_gold  ] = WGold;
        (*this)[f_bishop] = BBishop;
        (*this)[e_bishop] = WBishop;
        (*this)[f_horse ] = BHorse;
        (*this)[e_horse ] = WHorse;
        (*this)[f_rook  ] = BRook;
        (*this)[e_rook  ] = WRook;
        (*this)[f_dragon] = BDragon;
        (*this)[e_dragon] = WDragon;
    }
    Piece value(const int i) const {
        const auto it = find(i);
        if (it == std::end(*this))
            return PieceNone;
        return it->second;
    }
};
extern KPPBoardIndexStartToPiece g_kppBoardIndexStartToPiece;

template <typename Tl, typename Tr>
inline std::array<Tl, 2> operator += (std::array<Tl, 2>& lhs, const std::array<Tr, 2>& rhs) {
    lhs[0] += rhs[0];
    lhs[1] += rhs[1];
    return lhs;
}
template <typename Tl, typename Tr>
inline std::array<Tl, 2> operator -= (std::array<Tl, 2>& lhs, const std::array<Tr, 2>& rhs) {
    lhs[0] -= rhs[0];
    lhs[1] -= rhs[1];
    return lhs;
}

const int KPPIndicesMax = 2;
const int KKPIndicesMax = 2;
const int PPPIndicesMax = 2;

template <typename EvalElementType, typename PPPEvalElementType> struct EvaluatorBase {
    static const int R_Mid = 8; // 相対位置の中心のindex
    constexpr int MaxWeight() const { return 1; }
    constexpr int TurnWeight() const { return 8; }
    // 冗長に配列を確保しているが、対称な関係にある時は常に若いindexの方にアクセスすることにする。
    // 例えば kpp だったら、k が優先的に小さくなるようする。左右の対称も含めてアクセス位置を決める。
    // ただし、kkp に関する項目 (kkp, r_kkp_b, r_kkp_h) のみ、p は味方の駒として扱うので、k0 < k1 となるとは限らない。
    struct KPPElements {
        EvalElementType kpp[SquareNoLeftNum][fe_end][fe_end];
    };
    KPPElements kpps;

    struct KKPElements {
        EvalElementType kkp[SquareNoLeftNum][SquareNum][fe_end];
    };
    KKPElements kkps;

    struct PPPElements {
        PPPEvalElementType ppp[of_end][fe_end][fe_end]; // 最初のP要素は、味方の駒である事を前提にする。
    };
    PPPElements ppps;

    // これらは↑のメンバ変数に一次元配列としてアクセスする為のもの。
    // 配列の要素数は上のstructのサイズから分かるはずだが無名structなのでsizeof()使いにくいから使わない。
    // 先頭さえ分かれば良いので要素数1で良い。
    EvalElementType*    oneArrayKPP(const u64 i) { return reinterpret_cast<EvalElementType*   >(&kpps) + i; }
    EvalElementType*    oneArrayKKP(const u64 i) { return reinterpret_cast<EvalElementType*   >(&kkps) + i; }
    PPPEvalElementType* oneArrayPPP(const u64 i) { return reinterpret_cast<PPPEvalElementType*>(&ppps) + i; }

    // todo: これらややこしいし汚いので使わないようにする。
    //       型によっては kkps_begin_index などの値が異なる。
    //       ただ、end - begin のサイズは型によらず一定。
    constexpr size_t kpps_end_index() const { return sizeof(kpps)/sizeof(EvalElementType); }
    constexpr size_t kkps_end_index() const { return sizeof(kkps)/sizeof(EvalElementType); }
    constexpr size_t ppps_end_index() const { return sizeof(ppps)/sizeof(PPPEvalElementType); }

    // KPP に関する相対位置などの次元を落とした位置などのインデックスを全て返す。
    // 負のインデックスは、正のインデックスに変換した位置の点数を引く事を意味する。
    // 0 の時だけは正負が不明だが、0 は歩の持ち駒 0 枚を意味していて無効な値なので問題なし。
    // ptrdiff_t はインデックス、int は寄与の大きさ。MaxWeight分のいくつかで表記することにする。
    void kppIndices(ptrdiff_t ret[KPPIndicesMax], Square ksq, EvalIndex i, EvalIndex j) {
        int retIdx = 0;
        auto pushLastIndex = [&] {
            ret[retIdx++] = std::numeric_limits<ptrdiff_t>::max();
            assert(retIdx <= KPPIndicesMax);
        };
        // i == j のKP要素はKKPの方で行うので、こちらでは何も有効なindexを返さない。
        if (i == j) {
            pushLastIndex();
            return;
        }

        // 左右対称や、玉以外の2駒の入れ替えにより本質的に同じ位置関係のものは、常に同じアドレスを参照するようにする。
        const auto invk = inverseFile(ksq);
        const auto invi = inverseFileIndexIfOnBoard(i);
        const auto invj = inverseFileIndexIfOnBoard(j);
        auto p = std::min({&kpps.kpp[ksq][i][j], &kpps.kpp[ksq][j][i], &kpps.kpp[invk][invi][invj], &kpps.kpp[invk][invj][invi]});
        ret[retIdx++] = p - oneArrayKPP(0);
        pushLastIndex();
    }
    void kkpIndices(ptrdiff_t ret[KKPIndicesMax], Square ksq0, Square ksq1, EvalIndex i) {
        int retIdx = 0;
        auto pushLastIndex = [&] {
            ret[retIdx++] = std::numeric_limits<ptrdiff_t>::max();
            assert(retIdx <= KKPIndicesMax);
        };
        if (ksq0 == ksq1) {
            ret[retIdx++] = std::numeric_limits<ptrdiff_t>::max();
            assert(retIdx <= KKPIndicesMax);
            return;
        }
        if (i < fe_hand_end) { // i 持ち駒
            if (i == kppIndexBegin(i)) {
                pushLastIndex();
                return;
            }
        }
        else { // i 盤上
            const Square isq = static_cast<Square>(i - kppIndexBegin(i));
            if (ksq0 == isq || ksq1 == isq) {
                pushLastIndex();
                return;
            }
        }
        int sign = 1;
        if (!kppIndexIsBlack(i)) {
            const Square tmp = ksq0;
            ksq0 = inverse(ksq1);
            ksq1 = inverse(tmp);
            i = kppWhiteIndexToBlackIndex(i);
            sign = -1;
        }
        if (SQ59 < ksq0) {
            ksq0 = inverseFile(ksq0);
            ksq1 = inverseFile(ksq1);
            i = inverseFileIndexIfOnBoard(i);
        }
        else if (makeFile(ksq0) == File5 && SQ59 < ksq1) {
            ksq1 = inverseFile(ksq1);
            i = inverseFileIndexIfOnBoard(i);
        }
        else if (makeFile(ksq0) == File5 && makeFile(ksq1) == File5)
            i = inverseFileIndexIfLefterThanMiddle(i);
        ret[retIdx++] = sign*(&kkps.kkp[ksq0][ksq1][i] - oneArrayKKP(0));
        ret[retIdx++] = std::numeric_limits<ptrdiff_t>::max();
        assert(retIdx <= KKPIndicesMax);
    }
    void pppIndices(ptrdiff_t ret[PPPIndicesMax], EvalIndex i, EvalIndex j, EvalIndex k) {
        int retIdx = 0;
        auto pushLastIndex = [&] {
            ret[retIdx++] = std::numeric_limits<ptrdiff_t>::max();
            assert(retIdx <= PPPIndicesMax);
        };

        if (i == j || i == k || j == k) {
            pushLastIndex();
            return;
        }
        std::array<EvalIndex, 3> array = {{i, j, k}};
        // insertionSort 使うべきか？
        std::sort(std::begin(array), std::end(array)); // array[0] を最小の要素にする。i の駒の種類と、i が味方の駒か敵の駒かが決まる。
        int sign = 1;
        if (!kppIndexIsBlack(array[0])) {
            // array[0] は必ず味方の駒にする。
            array[0] = kppWhiteIndexToBlackIndex(array[0]);
            array[1] = kppIndexToOpponentIndex(array[1]);
            array[2] = kppIndexToOpponentIndex(array[2]);
            sign = -1;
        }
        std::array<EvalIndex, 3> invArray = {{inverseFileIndexIfOnBoard(array[0]),
                                              inverseFileIndexIfOnBoard(array[1]),
                                              inverseFileIndexIfOnBoard(array[2])}};
        std::sort(std::begin(array   ), std::end(array   ));
        std::sort(std::begin(invArray), std::end(invArray));
        const std::array<EvalIndex, 3> result = std::min(array, invArray); // 配列を辞書的に比較して最小のものを使う。
        assert(kppIndexIsBlack(result[0]));
        // result[0] を EvalIndexOnlyF になおす。
        const EvalIndexOnlyF of = evalIndexToEvalIndexOnlyF(result[0]);
        ret[retIdx++] = sign*(&ppps.ppp[of][result[1]][result[2]] - oneArrayPPP(0));
        pushLastIndex();
    }
    void clear() { memset(this, 0, sizeof(*this)); } // float 型とかだと規格的に 0 は保証されなかった気がするが実用上問題ないだろう。
};

using EvalElementType = std::array<s16, 2>;
using PPPEvalElementType = s16;
struct Evaluator : public EvaluatorBase<EvalElementType, PPPEvalElementType> {
    using Base = EvaluatorBase<EvalElementType, PPPEvalElementType>;
    static EvalElementType KPP[SquareNum][fe_end][fe_end];
    static EvalElementType KKP[SquareNum][SquareNum][fe_end];

    static std::string addSlashIfNone(const std::string& str) {
        std::string ret = str;
        if (ret == "")
            ret += ".";
        if (ret.back() != '/')
            ret += "/";
        return ret;
    }

    void setEvaluate() {
#if !defined LEARN
        SYNCCOUT << "info string start setting eval table" << SYNCENDL;
#endif
        // index が負の時、反対側の駒として扱うので、-index で要素にアクセスし、通常の位置関係には値を -1 倍する。(手番評価は常に加算)
#define FOO(indices, oneArray, sum)                                     \
        for (auto index : indices) {                                    \
            if (index == std::numeric_limits<ptrdiff_t>::max()) break;  \
            if (0 <= index) {                                           \
                sum[0] += static_cast<s64>((*oneArray( index))[0]);     \
                sum[1] += static_cast<s64>((*oneArray( index))[1]);     \
            }                                                           \
            else {                                                      \
                sum[0] -= static_cast<s64>((*oneArray(-index))[0]);     \
                sum[1] += static_cast<s64>((*oneArray(-index))[1]);     \
            }                                                           \
        }                                                               \
        sum[1] /= Base::TurnWeight();

#if defined _OPENMP
#pragma omp parallel
#endif
        // KPP
        {
#ifdef _OPENMP
#pragma omp for
#endif
            // OpenMP対応したら何故か ksq を Square 型にすると ++ksq が定義されていなくてコンパイルエラーになる。
            for (int ksq = SQ11; ksq < SquareNum; ++ksq) {
                // indices は更に for ループの外側に置きたいが、OpenMP 使っているとアクセス競合しそうなのでループの中に置く。
                ptrdiff_t indices[KPPIndicesMax];
                for (EvalIndex i = (EvalIndex)0; i < fe_end; ++i) {
                    for (EvalIndex j = (EvalIndex)0; j < fe_end; ++j) {
                        Base::kppIndices(indices, static_cast<Square>(ksq), i, j);
                        std::array<s64, 2> sum = {{}};
                        FOO(indices, Base::oneArrayKPP, sum);
                        KPP[ksq][i][j] += sum;
                    }
                }
            }
        }
        // KKP
        {
#ifdef _OPENMP
#pragma omp for
#endif
            for (int ksq0 = SQ11; ksq0 < SquareNum; ++ksq0) {
                ptrdiff_t indices[KKPIndicesMax];
                for (Square ksq1 = SQ11; ksq1 < SquareNum; ++ksq1) {
                    for (EvalIndex i = (EvalIndex)0; i < fe_end; ++i) {
                        Base::kkpIndices(indices, static_cast<Square>(ksq0), ksq1, i);
                        std::array<s64, 2> sum = {{}};
                        FOO(indices, Base::oneArrayKKP, sum);
                        KKP[ksq0][ksq1][i] += sum;
                    }
                }
            }
        }
#undef FOO

#if !defined LEARN
        SYNCCOUT << "info string end setting eval table" << SYNCENDL;
#endif
    }

    void init(const std::string& dirName, const bool Synthesized, const bool readBase = true) {
        // 合成された評価関数バイナリがあればそちらを使う。
        if (Synthesized) {
            if (readSynthesized(dirName))
                return;
        }
        if (readBase)
            clear();
        readSomeSynthesized(dirName);
        if (readBase)
            read(dirName);
        setEvaluate();
    }

#define ALL_SYNTHESIZED_EVAL {                  \
        FOO(KPP);                               \
        FOO(KKP);                               \
    }
    static bool readSynthesized(const std::string& dirName) {
#define FOO(x) {                                                        \
            std::ifstream ifs((addSlashIfNone(dirName) + #x "_synthesized.bin").c_str(), std::ios::binary); \
            if (ifs) ifs.read(reinterpret_cast<char*>(x), sizeof(x));   \
            else     return false;                                      \
        }
        ALL_SYNTHESIZED_EVAL;
#undef FOO
        return true;
    }
    static void writeSynthesized(const std::string& dirName) {
#define FOO(x) {                                                        \
            std::ofstream ofs((addSlashIfNone(dirName) + #x "_synthesized.bin").c_str(), std::ios::binary); \
            ofs.write(reinterpret_cast<char*>(x), sizeof(x));           \
        }
        ALL_SYNTHESIZED_EVAL;
#undef FOO
    }
    static void readSomeSynthesized(const std::string& dirName) {
#define FOO(x) {                                                        \
            std::ifstream ifs((addSlashIfNone(dirName) + #x "_some_synthesized.bin").c_str(), std::ios::binary); \
            if (ifs) ifs.read(reinterpret_cast<char*>(x), sizeof(x));   \
            else     memset(x, 0, sizeof(x));                           \
        }
        ALL_SYNTHESIZED_EVAL;
#undef FOO
    }
    static void writeSomeSynthesized(const std::string& dirName) {
#define FOO(x) {                                                        \
            std::ofstream ofs((addSlashIfNone(dirName) + #x "_some_synthesized.bin").c_str(), std::ios::binary); \
            ofs.write(reinterpret_cast<char*>(x), sizeof(x));           \
        }
        ALL_SYNTHESIZED_EVAL;
#undef FOO
    }
#undef ALL_SYNTHESIZED_EVAL

#define BASE_ONLINE {                           \
        FOO(kpps.kpp);                          \
        FOO(kkps.kkp);                          \
    }

#define READ_BASE_EVAL {                        \
        BASE_ONLINE;                            \
    }
#define WRITE_BASE_EVAL {                       \
        BASE_ONLINE;                            \
    }
    void read(const std::string& dirName) {
#define FOO(x) {                                                        \
            std::ifstream ifs((addSlashIfNone(dirName) + #x ".bin").c_str(), std::ios::binary); \
            ifs.read(reinterpret_cast<char*>(x), sizeof(x));            \
        }
        READ_BASE_EVAL;
#undef FOO
    }
    void write(const std::string& dirName) {
#define FOO(x) {                                                        \
            std::ofstream ofs((addSlashIfNone(dirName) + #x ".bin").c_str(), std::ios::binary); \
            ofs.write(reinterpret_cast<char*>(x), sizeof(x));           \
        }
        WRITE_BASE_EVAL;
#undef FOO
    }
#undef READ_BASE_EVAL
#undef WRITE_BASE_EVAL
};

extern const int kppArray[31];
extern const int kkpArray[15];
extern const int kppHandArray[ColorNum][HandPieceNum];

struct EvalSum {
#if defined USE_AVX2_EVAL
    EvalSum(const EvalSum& es) {
        _mm256_store_si256(&mm, es.mm);
    }
    EvalSum& operator = (const EvalSum& rhs) {
        _mm256_store_si256(&mm, rhs.mm);
        return *this;
    }
#elif defined USE_SSE_EVAL
    EvalSum(const EvalSum& es) {
        _mm_store_si128(&m[0], es.m[0]);
        _mm_store_si128(&m[1], es.m[1]);
    }
    EvalSum& operator = (const EvalSum& rhs) {
        _mm_store_si128(&m[0], rhs.m[0]);
        _mm_store_si128(&m[1], rhs.m[1]);
        return *this;
    }
#endif
    EvalSum() {}
    s32 sum(const Color c) const {
        const s32 scoreBoard = p[0][0] - p[1][0] + p[2][0];
        const s32 scoreTurn  = p[0][1] + p[1][1] + p[2][1];
        return (c == Black ? scoreBoard : -scoreBoard) + scoreTurn;
    }
    EvalSum& operator += (const EvalSum& rhs) {
#if defined USE_AVX2_EVAL
        mm = _mm256_add_epi32(mm, rhs.mm);
#elif defined USE_SSE_EVAL
        m[0] = _mm_add_epi32(m[0], rhs.m[0]);
        m[1] = _mm_add_epi32(m[1], rhs.m[1]);
#else
        p[0][0] += rhs.p[0][0];
        p[0][1] += rhs.p[0][1];
        p[1][0] += rhs.p[1][0];
        p[1][1] += rhs.p[1][1];
        p[2][0] += rhs.p[2][0];
        p[2][1] += rhs.p[2][1];
#endif
        return *this;
    }
    EvalSum& operator -= (const EvalSum& rhs) {
#if defined USE_AVX2_EVAL
        mm = _mm256_sub_epi32(mm, rhs.mm);
#elif defined USE_SSE_EVAL
        m[0] = _mm_sub_epi32(m[0], rhs.m[0]);
        m[1] = _mm_sub_epi32(m[1], rhs.m[1]);
#else
        p[0][0] -= rhs.p[0][0];
        p[0][1] -= rhs.p[0][1];
        p[1][0] -= rhs.p[1][0];
        p[1][1] -= rhs.p[1][1];
        p[2][0] -= rhs.p[2][0];
        p[2][1] -= rhs.p[2][1];
#endif
        return *this;
    }
    EvalSum operator + (const EvalSum& rhs) const { return EvalSum(*this) += rhs; }
    EvalSum operator - (const EvalSum& rhs) const { return EvalSum(*this) -= rhs; }

    // ehash 用。
    void encode() {
#if defined USE_AVX2_EVAL
        // EvalSum は atomic にコピーされるので key が合っていればデータも合っている。
#else
        key ^= data[0] ^ data[1] ^ data[2];
#endif
    }
    void decode() { encode(); }

    union {
        std::array<std::array<s32, 2>, 3> p;
        struct {
            u64 data[3];
            u64 key; // ehash用。
        };
#if defined USE_AVX2_EVAL
        __m256i mm;
#endif
#if defined USE_AVX2_EVAL || defined USE_SSE_EVAL
        __m128i m[2];
#endif
    };
};

class Position;
struct SearchStack;

const size_t EvaluateTableSize = 0x400000; // 134MB
//const size_t EvaluateTableSize = 0x2000000; // 1GB
//const size_t EvaluateTableSize = 0x10000000; // 8GB
//const size_t EvaluateTableSize = 0x20000000; // 17GB

using EvaluateHashEntry = EvalSum;
struct EvaluateHashTable : HashTable<EvaluateHashEntry, EvaluateTableSize> {};
extern EvaluateHashTable g_evalTable;

Score evaluateUnUseDiff(const Position& pos);
Score evaluate(Position& pos, SearchStack* ss);

#endif // #ifndef APERY_EVALUATE_HPP
