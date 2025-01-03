USE [master]
GO
/****** Object:  Database [ChatApp]    Script Date: 01/04/2025 11:15:00 ******/
CREATE DATABASE [ChatApp] ON  PRIMARY 
( NAME = N'ChatApp', FILENAME = N'C:\Program Files\Microsoft SQL Server\MSSQL10_50.MSSQLSERVER\MSSQL\DATA\ChatApp.mdf' , SIZE = 3072KB , MAXSIZE = UNLIMITED, FILEGROWTH = 1024KB )
 LOG ON 
( NAME = N'ChatApp_log', FILENAME = N'C:\Program Files\Microsoft SQL Server\MSSQL10_50.MSSQLSERVER\MSSQL\DATA\ChatApp_log.ldf' , SIZE = 1024KB , MAXSIZE = 2048GB , FILEGROWTH = 10%)
GO
ALTER DATABASE [ChatApp] SET COMPATIBILITY_LEVEL = 100
GO
IF (1 = FULLTEXTSERVICEPROPERTY('IsFullTextInstalled'))
begin
EXEC [ChatApp].[dbo].[sp_fulltext_database] @action = 'enable'
end
GO
ALTER DATABASE [ChatApp] SET ANSI_NULL_DEFAULT OFF
GO
ALTER DATABASE [ChatApp] SET ANSI_NULLS OFF
GO
ALTER DATABASE [ChatApp] SET ANSI_PADDING OFF
GO
ALTER DATABASE [ChatApp] SET ANSI_WARNINGS OFF
GO
ALTER DATABASE [ChatApp] SET ARITHABORT OFF
GO
ALTER DATABASE [ChatApp] SET AUTO_CLOSE OFF
GO
ALTER DATABASE [ChatApp] SET AUTO_CREATE_STATISTICS ON
GO
ALTER DATABASE [ChatApp] SET AUTO_SHRINK OFF
GO
ALTER DATABASE [ChatApp] SET AUTO_UPDATE_STATISTICS ON
GO
ALTER DATABASE [ChatApp] SET CURSOR_CLOSE_ON_COMMIT OFF
GO
ALTER DATABASE [ChatApp] SET CURSOR_DEFAULT  GLOBAL
GO
ALTER DATABASE [ChatApp] SET CONCAT_NULL_YIELDS_NULL OFF
GO
ALTER DATABASE [ChatApp] SET NUMERIC_ROUNDABORT OFF
GO
ALTER DATABASE [ChatApp] SET QUOTED_IDENTIFIER OFF
GO
ALTER DATABASE [ChatApp] SET RECURSIVE_TRIGGERS OFF
GO
ALTER DATABASE [ChatApp] SET  DISABLE_BROKER
GO
ALTER DATABASE [ChatApp] SET AUTO_UPDATE_STATISTICS_ASYNC OFF
GO
ALTER DATABASE [ChatApp] SET DATE_CORRELATION_OPTIMIZATION OFF
GO
ALTER DATABASE [ChatApp] SET TRUSTWORTHY OFF
GO
ALTER DATABASE [ChatApp] SET ALLOW_SNAPSHOT_ISOLATION OFF
GO
ALTER DATABASE [ChatApp] SET PARAMETERIZATION SIMPLE
GO
ALTER DATABASE [ChatApp] SET READ_COMMITTED_SNAPSHOT OFF
GO
ALTER DATABASE [ChatApp] SET HONOR_BROKER_PRIORITY OFF
GO
ALTER DATABASE [ChatApp] SET  READ_WRITE
GO
ALTER DATABASE [ChatApp] SET RECOVERY FULL
GO
ALTER DATABASE [ChatApp] SET  MULTI_USER
GO
ALTER DATABASE [ChatApp] SET PAGE_VERIFY CHECKSUM
GO
ALTER DATABASE [ChatApp] SET DB_CHAINING OFF
GO
EXEC sys.sp_db_vardecimal_storage_format N'ChatApp', N'ON'
GO
USE [ChatApp]
GO
/****** Object:  Table [dbo].[client]    Script Date: 01/04/2025 11:15:00 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[client](
	[username] [varchar](50) NOT NULL,
	[ip] [varchar](50) NULL,
	[port] [varchar](20) NULL,
	[password] [varchar](20) NULL,
	[is_online] [tinyint] NOT NULL,
 CONSTRAINT [PK_client] PRIMARY KEY CLUSTERED 
(
	[username] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]
GO
SET ANSI_PADDING OFF
GO
/****** Object:  Table [dbo].[messages]    Script Date: 01/04/2025 11:15:00 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[messages](
	[sender_id] [varchar](50) NOT NULL,
	[receiver_id] [varchar](50) NOT NULL,
	[message_text] [nvarchar](max) NOT NULL,
	[send_time] [datetime] NULL,
	[is_read] [bit] NULL
) ON [PRIMARY]
GO
SET ANSI_PADDING OFF
GO
/****** Object:  Table [dbo].[friend_relationship]    Script Date: 01/04/2025 11:15:00 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[friend_relationship](
	[user_id] [varchar](50) NOT NULL,
	[friend_id] [varchar](50) NOT NULL,
	[status] [tinyint] NULL
) ON [PRIMARY]
GO
SET ANSI_PADDING OFF
GO
/****** Object:  Default [DF__client__is_onlin__07020F21]    Script Date: 01/04/2025 11:15:00 ******/
ALTER TABLE [dbo].[client] ADD  DEFAULT ((0)) FOR [is_online]
GO
/****** Object:  Default [DF__messages__send_t__1FCDBCEB]    Script Date: 01/04/2025 11:15:00 ******/
ALTER TABLE [dbo].[messages] ADD  DEFAULT (getdate()) FOR [send_time]
GO
/****** Object:  Default [DF__messages__is_rea__20C1E124]    Script Date: 01/04/2025 11:15:00 ******/
ALTER TABLE [dbo].[messages] ADD  DEFAULT ((0)) FOR [is_read]
GO
/****** Object:  Default [DF__friend_re__statu__03317E3D]    Script Date: 01/04/2025 11:15:00 ******/
ALTER TABLE [dbo].[friend_relationship] ADD  DEFAULT ((0)) FOR [status]
GO
/****** Object:  ForeignKey [fk_receiver]    Script Date: 01/04/2025 11:15:00 ******/
ALTER TABLE [dbo].[messages]  WITH CHECK ADD  CONSTRAINT [fk_receiver] FOREIGN KEY([receiver_id])
REFERENCES [dbo].[client] ([username])
GO
ALTER TABLE [dbo].[messages] CHECK CONSTRAINT [fk_receiver]
GO
/****** Object:  ForeignKey [fk_sender]    Script Date: 01/04/2025 11:15:00 ******/
ALTER TABLE [dbo].[messages]  WITH CHECK ADD  CONSTRAINT [fk_sender] FOREIGN KEY([sender_id])
REFERENCES [dbo].[client] ([username])
GO
ALTER TABLE [dbo].[messages] CHECK CONSTRAINT [fk_sender]
GO
/****** Object:  ForeignKey [FK__friend_re__frien__0519C6AF]    Script Date: 01/04/2025 11:15:00 ******/
ALTER TABLE [dbo].[friend_relationship]  WITH CHECK ADD FOREIGN KEY([friend_id])
REFERENCES [dbo].[client] ([username])
GO
