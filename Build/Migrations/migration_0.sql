CREATE TABLE "Roles"
(
    "Id" INTEGER PRIMARY KEY AUTOINCREMENT,
    "DocumentId" character varying(1000) NOT NULL,
    "Document" jsonb,
    "RevisionNumber" bigint,
    "LastModified" text,
    "Checksum" bigint,
    "IsActive" boolean,
    "OwnerId" text,
    "OwnerName" text,
    "OperationDescription" text
);