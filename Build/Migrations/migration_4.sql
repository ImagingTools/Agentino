CREATE TABLE "UserSessions"(
	"AccessToken" UUID NOT NULL,
    "UserId" VARCHAR (1000) NOT NULL,
    "LastActivity" TIMESTAMP NOT NULL,
	PRIMARY KEY ("AccessToken"),
    FOREIGN KEY ("UserId") REFERENCES "Users" (UserId) ON DELETE CASCADE ON UPDATE CASCADE
);