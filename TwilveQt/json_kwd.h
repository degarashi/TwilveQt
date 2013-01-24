#ifndef JSON_KWD_H
#define JSON_KWD_H

namespace JSONKWD {
	namespace USER {
		const static char
		*Created_at = "created_at",
		*Default_image = "default_profile_image",
		*Profile_txtcolor = "profile_text_color",
		*Profile_bkcolor = "profile_background_color",
		*List_count = "listed_count",
		*Notifications = "notifications",
		*Following = "following",
		*Friends_count = "friends_count",
		*Timezone = "time_zone",
		*Description = "description",
		*Protected = "protected",
		*Url = "url",
		*Lang = "lang",
		*Id = "id",
		*Utc_offset = "utc_offset",
		*Profile_sidecolor = "profile_sidebar_fill_color",
		*Profile_url = "profile_image_url",
		*Follow_sent = "follow_request_sent",
		*Followers_count = "followers_count",
		*Name = "name",
		*Is_translator = "is_translator",
		*Profile_usebkimage = "profile_use_background_image",
		*Contributors_enabled = "contributors_enabled",
		*Profile_surl = "profile_image_url_https",
		*Profile_sidebdrcolor = "profile_sidebar_border_color",
		*IdStr = "id_str",
		*Screen_name = "screen_name",
		*GeoEnabled = "geo_enabled",
		*Verified = "verified",
		*Profile_bksurl = "profile_background_image_url_https",
		*Statuses_count = "statuses_count",
		*Default_prof = "default_profile",
		*Profile_bkurl = "profile_background_image_url",
		*Favorites_count = "favourites_count",
		*Show_inline = "show_all_inline_media";
	}
	namespace METADATA {
		const static char
		*RecentRTs = "recent_retweets",
		*ResultType = "result_type";
	}
	namespace TWEET {
		const static char
		*Created_at = "created_at",
		*Geo = "geo",
		*Id = "id",
		*Text = "text",
		*Source = "source",
		*IdStr = "id_str",

		// Home only
		*ReTweeted = "retweeted",
		*ReTweet_count = "retweet_count",
		*Coordinates = "coordinates",
		*Contributors = "contributors",
		*InReplyToScName = "in_reply_to_screen_name",
		*Place = "place",
		*InReplyToUserID = "in_reply_to_user_id",
		*InReplyToStatusIdStr = "in_reply_to_status_id_str",
		*InReplyToStatusId = "in_reply_to_status_id",
		*Favorited = "favorited",
		*InReplyToUserIdStr = "in_reply_to_user_id_str",
		*Truncated = "truncated",
		*ReTweeted_stat = "retweeted_status",
		*CurrentUserRetweet = "current_user_retweet",

		// Search only
		*ToUserName = "to_user_name",
		*LangCode = "iso_language_code",
		*Profile_imgurl = "profile_image_url",
		*FromUser = "from_user",
		*FromUserName = "from_user_name",
		*FromUserIdStr = "from_user_id_str",
		*FromUserId = "from_user_id",
		*Profile_imgsurl = "profile_image_url_https",
		*ToUser = "to_user",
		*ToUserID = "to_user_id",
		*ToUserIDStr = "to_user_id_str",
		*User = "user";
	}

	namespace SEARCH {
		const static char
		*Statuses = "statuses",
		*ScMetaData = "search_metadata",
		*MaxId = "max_id",
		*SinceId = "since_id",
		*RefreshUrl = "refresh_url",
		*NextResults = "next_results";
	}

	namespace APILIMIT {
		const static char
		*RemainingHits = "remaining_hits",
		*ResetTimeInSec = "reset_time_in_seconds",
		*HourlyLimit = "hourly_limit",
		*Photos = "photos",
		*ResetTime = "reset_time";

		namespace PHOTOS {
			const static char
			*RemainingHits = "remaining_hits",
			*ResetTimeInSec = "reset_time_in_seconds",
			*ResetTime = "reset_time",
			*DailyLimit = "daily_limit";
		}
	}
}

#endif // JSON_KWD_H
