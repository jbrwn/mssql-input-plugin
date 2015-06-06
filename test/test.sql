USE master
GO

IF db_id('mapnik_tmp_mssql_db') IS NOT NULL
    DROP DATABASE mapnik_tmp_mssql_db

CREATE DATABASE mapnik_tmp_mssql_db
GO

USE mapnik_tmp_mssql_db
GO

CREATE TABLE table1 
(
    id INT IDENTITY (1,1),
    geom GEOMETRY
)
GO

INSERT INTO table1 (geom) VALUES 
(geometry::STGeomFromText('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))', 4326))