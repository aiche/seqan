// ==========================================================================
//                 SeqAn - The Library for Sequence Analysis
// ==========================================================================
// Copyright (c) 2006-2010, Knut Reinert, FU Berlin
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Knut Reinert or the FU Berlin nor the names of
//       its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL KNUT REINERT OR THE FU BERLIN BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// ==========================================================================
// Author: Andreas Gogol-Doering <andreas.doering@mdc-berlin.de>
// ==========================================================================

#ifndef SEQAN_HEADER_FIND_BEGIN_H
#define SEQAN_HEADER_FIND_BEGIN_H


//////////////////////////////////////////////////////////////////////////////

namespace SEQAN_NAMESPACE_MAIN
{

//////////////////////////////////////////////////////////////////////////////
// Forward declarations of Pattern Specs that could be used prefix search

//see find_score.h
template <typename TScore, typename TSpec, typename TFindBeginPatternSpec>
struct DPSearch;

//see finder_myers_ukkonen.h
template <typename TSpec, typename THasState, typename TFindBeginPatternSpec>
struct Myers;


//____________________________________________________________________________

// TODO(holtgrew): Difference to DefaultPattern?
/**
.Metafunction.DefaultFindBeginPatternSpec:
..cat:Searching
..summary:Type of the default findBegin pattern specialization, given a score.
..signature:DefaultBeginPatternSpec<TScore>
..param.TScore:A score type.
...type:Class.Score
..returns.param.Type:The pattern specialization to use as a default.
..include:seqan/find.h
 */
template <typename TScore = EditDistanceScore, typename THasState = True>
struct DefaultFindBeginPatternSpec
{
	typedef DPSearch<TScore, FindPrefix, void> Type;
};

template <typename THasState>
struct DefaultFindBeginPatternSpec<EditDistanceScore, THasState>
{
	// typedef Myers<FindPrefix, void, True> Type;
	typedef Myers<FindPrefix, THasState, void> Type;
};

//____________________________________________________________________________
//must be implemented for all patterns that do approximate infix finding

template <typename TPattern>
struct FindBeginPatternSpec 
{
	typedef void Type; //void means: no find begin (see FindBegin_)
};

//____________________________________________________________________________
// Metafunction FindBeginPattern: pattern used to find the begin of an approximate match

template <typename TPattern>
struct FindBeginPattern
{
	typedef typename Needle<TPattern>::Type TNeedle_;
	typedef ModifiedString<TNeedle_, ModReverse> TReverseNeedle_;
	typedef typename FindBeginPatternSpec<TPattern>::Type TFindBeginPatternSpec_;
	typedef Pattern<TReverseNeedle_, TFindBeginPatternSpec_> Type;
};

//////////////////////////////////////////////////////////////////////////////
// Base class for approximate finders that manages findBegin stuff

template <typename TPattern, typename TFindBeginPatternSpec = typename FindBeginPatternSpec<TPattern>::Type >
struct FindBegin_
{
	typedef typename FindBeginPattern<TPattern>::Type TFindBeginPattern_;

	TFindBeginPattern_ data_findBeginPattern;
};

template <typename TPattern>
struct FindBegin_ <TPattern, void>
{
//need no findBegin if FindBeginPatternSpec is void
};


//////////////////////////////////////////////////////////////////////////////

template <typename TFindBeginPatternSpec>
struct FindBeginImpl_
{
	template <typename TPattern, typename TNeedle>
	static inline void
	_findBeginInit(TPattern & pattern, TNeedle & needle_)
	{
//		setNeedle(pattern.data_findBeginPattern, reverseString(needle(pattern)));
		setHost(needle(pattern.data_findBeginPattern), needle_);
		setScoringScheme(pattern.data_findBeginPattern, scoringScheme(pattern));
	}
//____________________________________________________________________________

	template <typename TFinder, typename TPattern, typename TLimit>
	static inline bool
	findBegin(TFinder & finder, TPattern & pattern, TLimit limit)
	{
		typedef typename FindBeginPattern<TPattern>::Type TFindBeginPattern;
		typedef typename Haystack<TFinder>::Type THaystack;
		typedef ModifiedString<THaystack, ModReverse> TReverseHaystack;
		typedef Finder<TReverseHaystack> TBeginFinder;
		typedef typename Position<THaystack>::Type TPosition;

		TFindBeginPattern & find_begin_pattern = pattern.data_findBeginPattern;
		setScoreLimit(find_begin_pattern, limit);

		//build begin_finder
		TBeginFinder begin_finder;
		THaystack & hayst = haystack(finder);
		setContainer(host(hostIterator(begin_finder)), hayst);
		TPosition begin_finder_beginPosition = position(finder);
		TPosition begin_finder_position;

		if (!finder._beginFind_called)
		{//start finding
			finder._beginFind_called = true;
			_setFinderLength(finder, 0);
			clear(begin_finder);
			begin_finder_position = begin_finder_beginPosition;
		}
		else
		{//resume finding
			_finderSetNonEmpty(begin_finder);
			SEQAN_ASSERT_GT(length(finder), 0u);
			begin_finder_position = endPosition(finder) - length(finder);
		}
		setPosition(host(hostIterator(begin_finder)), begin_finder_position);
		_setFinderEnd(begin_finder);
		_setFinderLength(begin_finder, length(finder));

		bool begin_found = find(begin_finder, find_begin_pattern);
		if (begin_found)
		{//new begin found: report in finder
			_setFinderLength(finder, begin_finder_beginPosition - position(host(hostIterator(begin_finder)))+1);
		}
		return begin_found;
	}
	template <typename TFinder, typename TPattern>
	static inline bool
	findBegin(TFinder & finder, TPattern & pattern)
	{
		return findBegin(finder, pattern, scoreLimit(pattern));
	}

//____________________________________________________________________________

	template <typename TPattern>
	static inline typename Value<typename ScoringScheme<TPattern>::Type>::Type
	getBeginScore(TPattern & pattern)
	{
		return getScore(pattern.data_findBeginPattern);
	}
};


// TODO(holtgrew): Copy-and-paste code from above.
template <typename THasState>
struct FindBeginImpl_<Myers<FindPrefix, THasState, void> >
{
	template <typename TPattern, typename TNeedle>
	static inline void
	_findBeginInit(TPattern & pattern, TNeedle & needle_)
	{
//		setNeedle(pattern.data_findBeginPattern, reverseString(needle(pattern)));
		setHost(pattern.data_findBeginPattern, reverseString(needle_));
//         _patternFirstInit(pattern.data_findBeginPattern, host(pattern));
	}
//____________________________________________________________________________

	template <typename TFinder, typename TPattern, typename TLimit>
	static inline bool
	findBegin(TFinder & finder, TPattern & pattern, TLimit limit)
	{
		typedef typename FindBeginPattern<TPattern>::Type TFindBeginPattern;
		typedef typename Haystack<TFinder>::Type THaystack;
		typedef ModifiedString<THaystack, ModReverse> TReverseHaystack;
		typedef Finder<TReverseHaystack> TBeginFinder;
		typedef typename Position<THaystack>::Type TPosition;

		TFindBeginPattern & find_begin_pattern = pattern.data_findBeginPattern;
		setScoreLimit(find_begin_pattern, limit);

		//build begin_finder
		TBeginFinder begin_finder;
		THaystack & hayst = haystack(finder);
		setContainer(host(hostIterator(begin_finder)), hayst);
		TPosition begin_finder_beginPosition = position(finder);
		TPosition begin_finder_position;

		if (!finder._beginFind_called)
		{//start finding
			finder._beginFind_called = true;
			_setFinderLength(finder, 0);
			clear(begin_finder);
			begin_finder_position = begin_finder_beginPosition;
		}
		else
		{//resume finding
			_finderSetNonEmpty(begin_finder);
			SEQAN_ASSERT_GT(length(finder), 0u);
			begin_finder_position = endPosition(finder) - length(finder);
		}
		setPosition(host(hostIterator(begin_finder)), begin_finder_position);
		_setFinderEnd(begin_finder);
		_setFinderLength(begin_finder, length(finder));

		bool begin_found = find(begin_finder, find_begin_pattern);
		if (begin_found)
		{//new begin found: report in finder
			_setFinderLength(finder, begin_finder_beginPosition - position(host(hostIterator(begin_finder)))+1);
		}
		return begin_found;
	}
	template <typename TFinder, typename TPattern>
	static inline bool
	findBegin(TFinder & finder, TPattern & pattern)
	{
		return findBegin(finder, pattern, scoreLimit(pattern));
	}

//____________________________________________________________________________

	template <typename TPattern>
	static inline typename Value<typename ScoringScheme<TPattern>::Type>::Type
	getBeginScore(TPattern & pattern)
	{
		return getScore(pattern.data_findBeginPattern);
	}
};

//default implementation of findBegin emulates the behaviour 
//of findBegin for approximate string matching algorithms
//for exact algorithms:
//the first call returns true ("begin position found")
//the next calls return false ("no further begin position found")
//no searching needed, since the exact algorithms set the 
//match length manually via "_setFinderLength" (it's the needle length)

template <>
struct FindBeginImpl_<void>
{
	template <typename TPattern, typename TNeedle>
	static inline void
	_findBeginInit(TPattern &, TNeedle &)
	{
	}

	template <typename TFinder, typename TPattern, typename TLimit>
	static inline bool
	findBegin(TFinder & finder, TPattern &, TLimit)
	{
		if (!finder._beginFind_called)
		{
			finder._beginFind_called = true;
			return true;
		}
		return false;
	}
	template <typename TFinder, typename TPattern>
	static inline bool
	findBegin(TFinder & finder, TPattern & pattern)
	{
		return findBegin(finder, pattern, 0);
	}

	template <typename TPattern>
	static inline typename Value<typename ScoringScheme<TPattern>::Type>::Type
	getBeginScore(TPattern & pattern)
	{
		return getScore(pattern);
	}
};


//////////////////////////////////////////////////////////////////////////////

//initialize pattern of begin finding
template <typename TPattern, typename TNeedle>
inline void
_findBeginInit(TPattern & pattern, TNeedle & needle_)
{
	typedef typename FindBeginPatternSpec<TPattern>::Type TFindBeginPatternSpec;
	return FindBeginImpl_<TFindBeginPatternSpec>::_findBeginInit(pattern, needle_);
}

//////////////////////////////////////////////////////////////////////////////

//find begin main interface 

/**
.Function.findBegin:
..summary:Search the begin of an approximate match.
..cat:Searching
..signature:findBegin(finder, pattern [, limit])
..class:Class.Finder
..param.finder:The @Class.Finder@ object to search through.
...type:Class.Finder
..param.pattern:The @Class.Pattern@ object to search for.
...remarks:This must be a pattern for approximate string matching.
...type:Class.Pattern
..param.limit:The score limit.
...default:The limit used during the last @Function.find@ call, see @Function.getScore@.
...remarks:All occurrences that score at least $limit$ are reported.
..returns:$boolean$ that indicates whether an begin position was found.
..remarks:The function @Function.find@ successfully called be called - that is an end position was found - before calling $findBegin$ to find a begin position.
..see:Function.getBeginScore
..see:Function.find
..include:seqan/find.h
*/
template <typename TFinder, typename TPattern>
inline bool
findBegin(TFinder & finder,
		  TPattern & pattern)
{
	typedef typename FindBeginPatternSpec<TPattern>::Type TFindBeginPatternSpec;
	return FindBeginImpl_<TFindBeginPatternSpec>::findBegin(finder, pattern);
}
template <typename TFinder, typename TPattern, typename TLimit>
inline bool
findBegin(TFinder & finder,
		  TPattern & pattern,
		  TLimit limit)
{
	typedef typename FindBeginPatternSpec<TPattern>::Type TFindBeginPatternSpec;
	return FindBeginImpl_<TFindBeginPatternSpec>::findBegin(finder, pattern, limit);
}

//////////////////////////////////////////////////////////////////////////////

/**.Function.getBeginScore
..cat:Searching
..summary:Score of the last match found by @Function.findBegin@ during approximate searching.
..signature:getBeginScore(pattern)
..class:Class.Finder
..param.pattern:A @Concept.PatternConcept|pattern@ that can be used for approximate searching.
...type:Spec.DPSearch
..returns:The score of the last match found using $pattern$.
...remarks:The value is set after a successfully call of @Function.findBegin@.
If no match was found, the value is undefined.
..see:Function.findBegin
..see:Function.getScore
*/

template <typename TPattern>
inline typename Value<typename ScoringScheme<TPattern>::Type>::Type
getBeginScore(TPattern & pattern)
{
	typedef typename FindBeginPatternSpec<TPattern>::Type TFindBeginPatternSpec;
	return FindBeginImpl_<TFindBeginPatternSpec>::getBeginScore(pattern);
}

//////////////////////////////////////////////////////////////////////////////

}// namespace SEQAN_NAMESPACE_MAIN

#endif //#ifndef SEQAN_HEADER_...
