/*
Copyright (C) 2018 Jonathon Ogden <jeog.dev@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses.
*/

#include <vector>
#include <unordered_map>
#include <iostream>
#include <tuple>
#include <cctype>
#include <string>

#include "../../include/_tdma_api.h"
#include "../../include/_get.h"

using std::string;
using std::vector;
using std::pair;
using std::tie;

namespace tdma {

using std::to_string;

class HistoricalGetterBaseImpl
        : public APIGetterImpl {
    string _symbol;
    FrequencyType _frequency_type;
    unsigned int _frequency;
    bool _extended_hours;

    virtual void
    build() = 0;

    void
    _throw_if_invalid_frequency(FrequencyType fty, int f) const
    {
        auto valid = VALID_FREQUENCIES_BY_FREQUENCY_TYPE.find(fty);
        assert( valid != end(VALID_FREQUENCIES_BY_FREQUENCY_TYPE) );
        if( valid->second.count(f) == 0 ){
            TDMA_API_THROW(ValueException,
                "invalid frequency(" + to_string(f)
                + ") for frequency type(" + to_string(fty) + ")"
                );
        }
    }

protected:
    HistoricalGetterBaseImpl( Credentials& creds,
                              const string& symbol,
                              FrequencyType frequency_type,
                              unsigned int frequency,
                              bool extended_hours )
        :
            APIGetterImpl(creds, data_api_on_error_callback),
            _symbol( util::toupper(symbol) ),
            _frequency_type(frequency_type),
            _frequency(frequency),
            _extended_hours(extended_hours)
        {
            if( symbol.empty() )
                TDMA_API_THROW(ValueException,"empty symbol");
            _throw_if_invalid_frequency(frequency_type, frequency);
        }

    vector<pair<string, string>>
    build_query_params() const
    {
        return { {"frequencyType", to_string(_frequency_type)},
                 {"frequency", to_string(_frequency)},
                 {"needExtendedHoursData", _extended_hours ? "true" : "false"} };
    }

public:
    typedef HistoricalGetterBase ProxyType;
    static const int TYPE_ID_LOW = TYPE_ID_GETTER_HISTORICAL_PERIOD;
    static const int TYPE_ID_HIGH = TYPE_ID_GETTER_HISTORICAL_RANGE;

    string
    get_symbol() const
    { return _symbol; }

    unsigned int
    get_frequency() const
    { return _frequency; }

    FrequencyType
    get_frequency_type() const
    { return _frequency_type; }

    bool
    is_extended_hours() const
    { return _extended_hours; }

    void
    set_symbol(const string& symbol)
    {
        if( symbol.empty() )
            TDMA_API_THROW(ValueException,"empty symbol");
        _symbol = util::toupper(symbol);
        build();
    }

    void
    set_extended_hours(bool extended_hours)
    {
        _extended_hours = extended_hours;
        build();
    }

    void
    set_frequency(FrequencyType frequency_type, unsigned int frequency)
    {
        _throw_if_invalid_frequency(frequency_type, frequency);
        _frequency_type = frequency_type;
        _frequency = frequency;
        build();
    }

};


class HistoricalPeriodGetterImpl
        : public HistoricalGetterBaseImpl {
    PeriodType _period_type;
    unsigned int _period;
    long long _msec_since_epoch;

    void
    _build()
    {
        vector<pair<string,string>> params( build_query_params() );
        params.emplace_back( "periodType", to_string(_period_type) );
        params.emplace_back( "period", to_string(_period) );

        /* FEATURE FIX - Oct 25 2018
         *
         * Add the ability to pass a start OR end datetime to the period.
         */
        if( _msec_since_epoch != 0 ){
            string f = _msec_since_epoch > 0 ? "endDate" : "startDate";
            params.emplace_back(
                f, to_string( std::llabs(_msec_since_epoch) )
            );
        }

        string qstr = util::build_encoded_query_str(params);
        string url = URL_MARKETDATA + util::url_encode(get_symbol())
                     + "/pricehistory?" + qstr;
        APIGetterImpl::set_url(url);
    }

    /*virtual*/ void
    build()
    { _build(); }

    void
    _throw_if_invalid_frequency_type(PeriodType pty, FrequencyType fty) const
    {
        auto valid = VALID_FREQUENCY_TYPES_BY_PERIOD_TYPE.find(pty);
        assert( valid != end(VALID_FREQUENCY_TYPES_BY_PERIOD_TYPE) );
        if( valid->second.count(fty) == 0 ){
            TDMA_API_THROW(ValueException,
                "invalid frequency type(" + to_string(fty)
                + ") for period type(" + to_string(pty) + ")"
                );
        }
    }

    void
    _throw_if_invalid_period(PeriodType pty, int p) const
    {
        auto valid = VALID_PERIODS_BY_PERIOD_TYPE.find(pty);
        assert( valid != end(VALID_PERIODS_BY_PERIOD_TYPE) );
        if( valid->second.count(p) == 0 ){
            TDMA_API_THROW(ValueException,
                "invalid period(" + to_string(p)
                + ") for period type(" + to_string(pty) + ")"
                );
        }
    }

public:
    typedef HistoricalPeriodGetter ProxyType;
    static const int TYPE_ID_LOW = TYPE_ID_GETTER_HISTORICAL_PERIOD;
    static const int TYPE_ID_HIGH = TYPE_ID_GETTER_HISTORICAL_PERIOD;

    HistoricalPeriodGetterImpl( Credentials& creds,
                                const string& symbol,
                                PeriodType period_type,
                                unsigned int period,
                                FrequencyType frequency_type,
                                unsigned int frequency,
                                bool extended_hours,
                                long long msec_since_epoch )
    :
        HistoricalGetterBaseImpl(creds, symbol, frequency_type, frequency,
                                 extended_hours),
        _period_type(period_type),
        _period(period),
        _msec_since_epoch(msec_since_epoch)
    {
        _throw_if_invalid_frequency_type(period_type, frequency_type);
        _throw_if_invalid_period(period_type, period);
        _build();
    }

    PeriodType
    get_period_type() const
    { return _period_type; }

    unsigned int
    get_period() const
    { return _period; }

    void
    set_period(PeriodType period_type, unsigned int period)
    {
        _throw_if_invalid_period(period_type, period);
        _period_type = period_type;
        _period = period;
        build();
    }

    void
    set_msec_since_epoch( long long msec_since_epoch )
    {
        _msec_since_epoch = msec_since_epoch;
        build();
    }

    long long
    get_msec_since_epoch() const
    { return _msec_since_epoch; }

    string
    get()
    {
        _throw_if_invalid_frequency_type(_period_type, get_frequency_type());
        /* NOTE - we have to wait to check frequency type against period_type
         *        until here to allow the client to make both set calls without
         *        the first triggering an exception from the stale values
         *        to be changed by the second call */
        return HistoricalGetterBaseImpl::get();
    }
};


class HistoricalRangeGetterImpl
        : public HistoricalGetterBaseImpl {
    unsigned long long _start_msec_since_epoch;
    unsigned long long _end_msec_since_epoch;

    void
    _build()
    {
        vector<pair<string,string>> params( build_query_params() );
        params.emplace_back( "startDate", to_string(_start_msec_since_epoch) );
        params.emplace_back( "endDate", to_string(_end_msec_since_epoch) );

        /* BUG FIX - Oct 25 2018
         *
         * contra example #4 in docs, if we use a daily/weekly/monthy frequency
         * type we need to specify a valid period type to override the default
         * value of 'day' which is invalid for those frequency types
         *
         * PeriodType::year is valid for all three
         */
        if( get_frequency_type() != FrequencyType::minute ){
            params.emplace_back( "periodType", to_string(PeriodType::year) );
        }

        string qstr = util::build_encoded_query_str(params);
        string url = URL_MARKETDATA + util::url_encode(get_symbol())
                     + "/pricehistory?" + qstr;
        APIGetterImpl::set_url(url);
    }

    /*virtual*/ void
    build()
    { _build(); }

public:
    typedef HistoricalRangeGetter ProxyType;
    static const int TYPE_ID_LOW = TYPE_ID_GETTER_HISTORICAL_RANGE;
    static const int TYPE_ID_HIGH = TYPE_ID_GETTER_HISTORICAL_RANGE;

    HistoricalRangeGetterImpl( Credentials& creds,
                               const string& symbol,
                               FrequencyType frequency_type,
                               unsigned int frequency,
                               unsigned long long start_msec_since_epoch,
                               unsigned long long end_msec_since_epoch,
                               bool extended_hours )
        :
            HistoricalGetterBaseImpl(creds, symbol, frequency_type, frequency,
                                     extended_hours),
            _start_msec_since_epoch(start_msec_since_epoch),
            _end_msec_since_epoch(end_msec_since_epoch)
        {
            _build();
        }

   unsigned long long
   get_end_msec_since_epoch() const
   { return _end_msec_since_epoch; }

   unsigned long long
   get_start_msec_since_epoch() const
   { return _start_msec_since_epoch; }

   void
   set_end_msec_since_epoch(unsigned long long end_msec_since_epoch)
   {
       _end_msec_since_epoch = end_msec_since_epoch;
       build();
   }

   void
   set_start_msec_since_epoch(unsigned long long start_msec_since_epoch)
   {
       _start_msec_since_epoch = start_msec_since_epoch;
       build();
   }
};

} /* tdma */


using namespace tdma;

int
HistoricalGetterBase_GetSymbol_ABI( Getter_C *pgetter,
                                    char **buf,
                                    size_t *n,
                                    int allow_exceptions )
{
    return ImplAccessor<char**>::template get<HistoricalGetterBaseImpl>(
        pgetter, &HistoricalGetterBaseImpl::get_symbol, buf, n, allow_exceptions
        );
}

int
HistoricalGetterBase_SetSymbol_ABI( Getter_C *pgetter,
                                    const char *symbol,
                                    int allow_exceptions )
{
    return ImplAccessor<char**>::template set<HistoricalGetterBaseImpl>(
        pgetter, &HistoricalGetterBaseImpl::set_symbol, symbol, allow_exceptions
        );
}

int
HistoricalGetterBase_GetFrequency_ABI( Getter_C *pgetter,
                                       unsigned int *frequency,
                                       int allow_exceptions )
{
    return ImplAccessor<unsigned int>::template
        get<HistoricalGetterBaseImpl>(
            pgetter, &HistoricalGetterBaseImpl::get_frequency, frequency,
            "frequency", allow_exceptions
        );
}

int
HistoricalGetterBase_GetFrequencyType_ABI( Getter_C *pgetter,
                                           int *frequency_type,
                                           int allow_exceptions )
{
    return ImplAccessor<int>::template
        get<HistoricalGetterBaseImpl, FrequencyType>(
            pgetter, &HistoricalGetterBaseImpl::get_frequency_type,
            frequency_type, "frequency_type", allow_exceptions
        );
}

int
HistoricalGetterBase_IsExtendedHours_ABI( Getter_C *pgetter,
                                          int *is_extended_hours,
                                          int allow_exceptions )
{
    return ImplAccessor<int>::template
        get<HistoricalGetterBaseImpl, bool>(
            pgetter, &HistoricalGetterBaseImpl::is_extended_hours,
            is_extended_hours, "is_extended_hours", allow_exceptions
        );
}

int
HistoricalGetterBase_SetExtendedHours_ABI( Getter_C *pgetter,
                                           int is_extended_hours,
                                           int allow_exceptions )
{
    return ImplAccessor<int>::template
        set<HistoricalGetterBaseImpl, bool>(
            pgetter, &HistoricalGetterBaseImpl::set_extended_hours,
            is_extended_hours, allow_exceptions
        );
}

int
HistoricalGetterBase_SetFrequency_ABI( Getter_C *pgetter,
                                       int frequency_type,
                                       unsigned int frequency,
                                       int allow_exceptions )
{
    CHECK_ENUM(FrequencyType, frequency_type, allow_exceptions);

    return ImplAccessor<int>::template
        set<HistoricalGetterBaseImpl, FrequencyType>(
            pgetter, &HistoricalGetterBaseImpl::set_frequency, frequency_type,
            frequency, allow_exceptions
            );
}

/* HistoricalPeriodGetter */
int
HistoricalPeriodGetter_Create_ABI( struct Credentials *pcreds,
                                   const char* symbol,
                                   int period_type,
                                   unsigned int period,
                                   int frequency_type,
                                   unsigned int frequency,
                                   int extended_hours,
                                   long long msec_since_epoch,
                                   HistoricalPeriodGetter_C *pgetter,
                                   int allow_exceptions )
{
    using ImplTy = HistoricalPeriodGetterImpl;

    int err = getter_is_creatable<ImplTy>(pcreds, pgetter, allow_exceptions);
    if( err )
        return err;

    CHECK_ENUM_KILL_PROXY( FrequencyType, frequency_type, allow_exceptions,
                               pgetter );
    CHECK_ENUM_KILL_PROXY( PeriodType, period_type, allow_exceptions,
                               pgetter );
    CHECK_PTR_KILL_PROXY(symbol, "symbol", allow_exceptions, pgetter);

    static auto meth = +[]( Credentials *c, const char* s, int pt,
                            unsigned int p, int ft, unsigned int f, int eh,
                            long long mse ){
        return new ImplTy( *c, s, static_cast<PeriodType>(pt), p,
                           static_cast<FrequencyType>(ft), f,
                           static_cast<bool>(eh), mse );
    };

    ImplTy *obj;
    tie(obj, err) = CallImplFromABI( allow_exceptions, meth, pcreds, symbol,
                                     period_type, period, frequency_type,
                                     frequency, extended_hours,
                                     msec_since_epoch );
    if( err ){
        kill_proxy(pgetter);
        return err;
    }

    pgetter->obj = reinterpret_cast<void*>(obj);
    assert( ImplTy::TYPE_ID_LOW == ImplTy::TYPE_ID_HIGH );
    pgetter->type_id = ImplTy::TYPE_ID_LOW;
    return 0;
}

int
HistoricalPeriodGetter_Destroy_ABI( HistoricalPeriodGetter_C *pgetter,
                                    int allow_exceptions )
{ return destroy_proxy<HistoricalPeriodGetterImpl>(pgetter, allow_exceptions); }

int
HistoricalPeriodGetter_GetPeriodType_ABI( HistoricalPeriodGetter_C *pgetter,
                                          int *period_type,
                                          int allow_exceptions )
{
    return ImplAccessor<int>::template
        get<HistoricalPeriodGetterImpl, PeriodType>(
            pgetter, &HistoricalPeriodGetterImpl::get_period_type,
            period_type, "period_type", allow_exceptions
        );
}


int
HistoricalPeriodGetter_GetPeriod_ABI( HistoricalPeriodGetter_C *pgetter,
                                      unsigned int *period,
                                      int allow_exceptions )
{
    return ImplAccessor<unsigned int>::template
        get<HistoricalPeriodGetterImpl>(
            pgetter, &HistoricalPeriodGetterImpl::get_period,
            period, "period", allow_exceptions
        );
}

int
HistoricalPeriodGetter_SetPeriod_ABI( HistoricalPeriodGetter_C *pgetter,
                                      int period_type,
                                      unsigned int period,
                                      int allow_exceptions )
{
    CHECK_ENUM(PeriodType, period_type, allow_exceptions);

    return ImplAccessor<int>::template
        set<HistoricalPeriodGetterImpl, PeriodType>(
            pgetter, &HistoricalPeriodGetterImpl::set_period, period_type,
            period, allow_exceptions
            );
}

int
HistoricalPeriodGetter_SetMSecSinceEpoch_ABI(
    HistoricalPeriodGetter_C *pgetter,
    long long msec_since_epoch,
    int allow_exceptions )
{
    return ImplAccessor<long long>::template
        set<HistoricalPeriodGetterImpl>(
            pgetter, &HistoricalPeriodGetterImpl::set_msec_since_epoch,
            msec_since_epoch, allow_exceptions
            );
}

int
HistoricalPeriodGetter_GetMSecSinceEpoch_ABI(
    HistoricalPeriodGetter_C *pgetter,
    long long *msec_since_epoch,
    int allow_exceptions )
{
    return ImplAccessor<long long>::template
        get<HistoricalPeriodGetterImpl>(
            pgetter, &HistoricalPeriodGetterImpl::get_msec_since_epoch,
            msec_since_epoch, "msec_since_epoch", allow_exceptions
        );

}


/* HistoricalRangeGetter */
int
HistoricalRangeGetter_Create_ABI( struct Credentials *pcreds,
                                  const char* symbol,
                                  int frequency_type,
                                  unsigned int frequency,
                                  unsigned long long start_msec_since_epoch,
                                  unsigned long long end_msec_since_epoch,
                                  int extended_hours,
                                  HistoricalRangeGetter_C *pgetter,
                                  int allow_exceptions )
{
    using ImplTy = HistoricalRangeGetterImpl;

    int err = getter_is_creatable<ImplTy>(pcreds, pgetter, allow_exceptions);
    if( err )
        return err;

    CHECK_ENUM_KILL_PROXY( FrequencyType, frequency_type, allow_exceptions,
                               pgetter );
    CHECK_PTR_KILL_PROXY(symbol, "symbol", allow_exceptions, pgetter);

    static auto meth = +[]( Credentials *c, const char* s, int ft,
                            unsigned int f, unsigned long long sm,
                            unsigned long long em, int eh ){
        return new ImplTy( *c, s, static_cast<FrequencyType>(ft), f, sm, em,
                           static_cast<bool>(eh) );
    };

    ImplTy *obj;
    tie(obj, err) = CallImplFromABI( allow_exceptions, meth, pcreds, symbol,
                                     frequency_type, frequency,
                                     start_msec_since_epoch, end_msec_since_epoch,
                                     extended_hours );

    if( err ){
        kill_proxy(pgetter);
        return err;
    }

    pgetter->obj = reinterpret_cast<void*>(obj);
    assert( ImplTy::TYPE_ID_LOW == ImplTy::TYPE_ID_HIGH );
    pgetter->type_id = ImplTy::TYPE_ID_LOW;
    return 0;
}

int
HistoricalRangeGetter_Destroy_ABI( HistoricalRangeGetter_C *pgetter,
                                   int allow_exceptions )
{ return destroy_proxy<HistoricalRangeGetterImpl>(pgetter, allow_exceptions); }

int
HistoricalRangeGetter_GetEndMSecSinceEpoch_ABI(
    HistoricalRangeGetter_C *pgetter,
    unsigned long long *end_msec,
    int allow_exceptions )
{
    return ImplAccessor<unsigned long long>::template
        get<HistoricalRangeGetterImpl>(
            pgetter, &HistoricalRangeGetterImpl::get_end_msec_since_epoch,
            end_msec, "end_msec", allow_exceptions
            );
}

int
HistoricalRangeGetter_SetEndMSecSinceEpoch_ABI(
    HistoricalRangeGetter_C *pgetter,
    unsigned long long end_msec,
    int allow_exceptions )
{
    return ImplAccessor<unsigned long long>::template
        set<HistoricalRangeGetterImpl>(
            pgetter, &HistoricalRangeGetterImpl::set_end_msec_since_epoch,
            end_msec, allow_exceptions
            );
}


int
HistoricalRangeGetter_GetStartMSecSinceEpoch_ABI(
    HistoricalRangeGetter_C *pgetter,
    unsigned long long *start_msec,
    int allow_exceptions )
{
    return ImplAccessor<unsigned long long>::template
        get<HistoricalRangeGetterImpl>(
            pgetter, &HistoricalRangeGetterImpl::get_start_msec_since_epoch,
            start_msec, "start_msec", allow_exceptions
            );
}

int
HistoricalRangeGetter_SetStartMSecSinceEpoch_ABI(
    HistoricalRangeGetter_C *pgetter,
    unsigned long long start_msec,
    int allow_exceptions )
{
    return ImplAccessor<unsigned long long>::template
        set<HistoricalRangeGetterImpl>(
            pgetter, &HistoricalRangeGetterImpl::set_start_msec_since_epoch,
            start_msec, allow_exceptions
            );
}

