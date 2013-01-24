#ifndef API_KWD_H
#define API_KWD_H
#include <QString>

namespace TWAPIKWD {
	namespace SC_SEARCH {
		const static char
		*Q = "q",
		*Callback = "callback",
		*Geocode = "geocode",
		*Lang = "lang",
		*Locale = "locale",
		*Page = "page",
		*ResultType = "result_type",
		*Rpp = "rpp",
		*ShowUser = "show_user",
		*Until = "until",
		*SinceId = "since_id",
		*MaxId = "max_id",
		*IncludeEntities = "include_entities";
	}
	namespace TW_UPDATE {
		const static char
		*Status = "status",
		*InReplyToStId = "in_reply_to_status_id",
		*Latitude = "lat",
		*Long = "long",
		*PlaceId = "place_id",
		*DispCoord = "display_coordinates",
		*TrimUser = "trim_user",
		*IncludeEnt = "trim_user";
	}
	namespace TL_HOME {
		const static char
		*Count = "count",
		*SinceId = "since_id",
		*MaxId = "max_id",
		*Page = "page",
		*TrimUser = "trim_user",
		*IncludeRTs = "include_rts",
		*IncludeEnt = "include_entities",
		*ExcludeReplys = "exclude_replies",
		*ContributorDetails = "contributor_details";
	}
	namespace TL_MENTIONS {
		const static char
		*Count = "count",
		*SinceId = "since_id",
		*MaxId = "max_id",
		*Page = "page",
		*TrimUser = "trim_user",
		*IncludeRTs = "include_rts",
		*IncludeEnts = "include_entities",
		*Contributor = "contributor_details";
	}
	namespace FV_CREATE {
		const static char
		*Id = "id",
		*IncludeEnts = "include_entities";
	}
	namespace FV_DESTROY {
		const static char
		*Id = "id";
	}
	namespace US_LOOKUP {
		const static char
		*ScreenName = "screen_name",
		*UserId = "user_id",
		*IncludeEnt = "include_entities";
	}
	namespace TL_USER {
		const static char
		*UserId = "user_id",
		*ScreenName = "screen_name",
		*SinceId = "since_id",
		*Count = "count",
		*MaxId = "max_id",
		*Page = "page",
		*TrimUser = "trim_user",
		*IncludeRTs = "include_rts",
		*IncludeEnts = "include_entities",
		*ExcludeReplies = "exclude_replies",
		*Contributor = "contributor_details";
	}
	namespace FR_FRIENDS {
		const static char
		*UserId = "user_id",
		*ScreenName = "screen_name",
		*Cursor = "cursor",
		*StringifyIds = "stringify_ids";
	}
	namespace FR_CREATE {
		const static char
		*ScreenName = "screen_name",
		*UserId = "user_id",
		*Follow = "follow";
	}
	namespace FR_DESTROY {
		const static char
		*UserId = "user_id",
		*ScreenName = "screen_name",
		*IncludeEnts = "include_entities";
	}
	namespace FR_SHOW {
		const static char
		*SourceId = "source_id",
		*SourceName = "source_screen_name",
		*TargetId = "target_id",
		*TargetName = "target_screen_name";
	}
	namespace BL_EXISTS {
		const static char
		*ScreenName = "screen_name",
		*UserId = "user_id",
		*IncludeEnts = "include_entities",
		*SkipStats = "skip_status";
	}
	namespace TW_SHOW {
		const static char
		*IncludeMyRetweet = "include_my_retweet",
		*IncludeEntities = "include_entities",
		*TrimUser = "trim_user";
	}
}
namespace TWAPI {
	const static QString
	_SEARCH("search.twitter.com"),
	_API("api.twitter.com"),
	_STREAM("stream.twitter.com");

	struct TWApiInfo {
		bool    _bGET, _bLimited;
		int     _iAuth;
		QString _host, _path;

		// Pathに含まれるのは1つの(文字列 or 数値)
		QString path() const;
		QString path(const QString& str) const;
		QString path(int64_t num) const;

		bool isGET() const;
		int requireAuth() const;
		const QString& host() const;
	};
	enum INDEX {
		TL_HOME,
		TL_MENTIONS,
		TL_RETWEETEDBYME,
		TL_RETWEETEDTOME,
		TL_RETWEETSOFME,
		TL_USER,
		TL_RETWEETEDTOUSER,
		TL_RETWEETEDBYUSER,

		TW_DESTROY,
		TW_RETWEET,
		TW_UPDATE,
		TW_SHOW,

		SC_SEARCH,

		FV_CREATE,
		FV_DESTROY,

		US_LOOKUP,

		FR_FRIENDS,
		FR_FOLLOWERS,
		FR_CREATE,
		FR_DESTROY,
		FR_SHOW,

		AC_LIMIT,
		AC_CREDENTIALS,

		BL_EXISTS,
		BL_CREATE,
		BL_DESTROY,
	};

	// [bGet, bLimited, iAuth, host, path]
	const static TWApiInfo INFO[] = {
		// Timelines
		{true, true, 2, _API, "/1.1/statuses/home_timeline.json"},
		{true, true, 2, _API, "/1.1/statuses/mentions_timeline.json"},
		{true, true, 2, _API, "/1/statuses/retweeted_by_me.json"},
		{true, true, 2, _API, "/1/statuses/retweeted_to_me.json"},
		{true, true, 2, _API, "/1.1/statuses/retweets_of_me.json"},
		{true, true, 1, _API, "/1.1/statuses/user_timeline.json"},
		{true, true, 1, _API, "/1/statuses/retweeted_to_user.json"},
		{true, false, 1, _API, "/1/statuses/retweeted_by_user.json"},
		// Tweets
		{false, false, 2, _API, "/1.1/statuses/destroy/%1.json"},
		{false, true, 2, _API, "/1.1/statuses/retweet/%1.json"},
		{false, false, 2, _API, "/1.1/statuses/update.json"},
		{true, true, 2, _API, "/1.1/statuses/show/%1.json"},
		// Search
		{true, true, 2, _API, "/1.1/search/tweets.json"},
		// Favorites
		{false, false, 2, _API, "/1/favorites/create/%1.json"},
		{false, false, 2, _API, "/1/favorites/destroy/%1.json"},
		// User
		{true, true, 1, _API, "/1/users/lookup.json"},
		// Friends
		{true, true, 1, _API, "/1/friends/ids.json"},
		{true, true, 1, _API, "/1/followers/ids.json"},
		{false, false, 2, _API, "/1/friendships/create.json"},
		{false, false, 2, _API, "/1/friendships/destroy.json"},
		{true, true, 1, _API, "/1.1/friendships/show.json"},
		// Accounts
		{true, true, 2, _API, "/1/account/rate_limit_status.json"},
		{true, true, 2, _API, "/1/account/verify_credentials.json"},
		// Block
		{true, true, 2, _API, "/1/blocks/exists.json"},
		{false, true, 2, _API, "/1/blocks/create.json"},
		{false, false, 2, _API, "/1/blocks/destroy.json"},
	};
}

#endif // API_KWD_H
