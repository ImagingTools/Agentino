CREATE TABLE "Messages"
(
    "Id" INTEGER PRIMARY KEY AUTOINCREMENT,
    "DocumentId" character varying(1000) NOT NULL,
    "Document" jsonb,
    "RevisionNumber" bigint,
    "LastModified" timestamp without time zone,
    "Checksum" bigint,
    "IsActive" boolean
);